#include "engine/renderer/SceneRenderer.h"

#include <glad/glad.h>

#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include "engine/Log.h"
#include "engine/renderer/RenderCommand.h"

namespace {
constexpr const char* kShadowVertexSource = R"(#version 330 core
layout(location = 0) in vec3 a_Position;

uniform mat4 uLightSpaceMatrix;
uniform mat4 uModel;

void main() {
    vec4 new_pos = uLightSpaceMatrix * uModel * vec4(a_Position, 1.0);
    gl_Position = new_pos;
}
)";

constexpr const char* kShadowFragmentSource = R"(#version 330 core
void main() {
    // depth only
}
)";

// Instanced shadow shader - reads transform from instance buffer
constexpr const char* kInstancedShadowVertexSource = R"(#version 330 core
layout(location = 0) in vec3 a_Position;

// Per-instance data (Mat4 uses locations 3-6)
layout(location = 3) in mat4 a_InstanceTransform;

uniform mat4 uLightSpaceMatrix;

void main() {
    vec4 world_pos = a_InstanceTransform * vec4(a_Position, 1.0);
    gl_Position = uLightSpaceMatrix * world_pos;
}
)";
}  // namespace

namespace se {

SceneRenderer::SceneRenderer() {}

SceneRenderer::~SceneRenderer() {
    if (initialized_) { Shutdown(); }
}

void SceneRenderer::Init() {
    if (initialized_) {
        SE_LOG_WARN("SceneRenderer already initialized");
        return;
    }

    SE_LOG_INFO("Initializing SceneRenderer");
    InitializeShadowResources();
    occlusionCuller_.Init();
    initialized_ = true;
}

void SceneRenderer::Shutdown() {
    if (!initialized_) return;

    SE_LOG_INFO("Shutting down SceneRenderer");
    occlusionCuller_.Shutdown();
    DestroyShadowResources();
    initialized_ = false;
}

void SceneRenderer::BeginScene(const Camera& camera, const Matrix4& projection) {
    sceneData_.ViewMatrix             = camera.getViewMatrix();
    sceneData_.ProjectionMatrix       = projection;
    sceneData_.view_projection_matrix = projection * sceneData_.ViewMatrix;
    sceneData_.Submissions.clear();
    instancedSubmissions_.clear();

    // Prepare directional light data and shadow matrix
    if (!sceneData_.directional_light.Active) {
        sceneData_.directional_light.Direction = Vector3(0.0f, -1.0f, 0.0f);
        sceneData_.directional_light.Intensity = 0.0f;
        sceneData_.directional_light.Color     = Vector3(1.0f);
        sceneData_.ShadowsEnabled              = false;
        sceneData_.LightSpaceMatrix            = Matrix4(1.0f);
    } else {
        Vector3 lightDir = sceneData_.directional_light.Direction;
        if (glm::length(lightDir) <= 0.0f) { lightDir = Vector3(0.0f, -1.0f, 0.0f); }
        lightDir                               = glm::normalize(lightDir);
        sceneData_.directional_light.Direction = lightDir;

        sceneData_.ShadowsEnabled = sceneData_.directional_light.CastShadows &&
                                    sceneData_.directional_light.Intensity > 0.0f;

        if (sceneData_.ShadowsEnabled) {
            // Shadow frustum follows camera position for consistent shadow coverage
            Vector3       cameraPos  = glm::inverse(sceneData_.ViewMatrix)[3];
            const Vector3 focusPoint = Vector3(cameraPos.x, 0.0f, cameraPos.z);
            const Vector3 lightPos   = focusPoint - lightDir * sceneData_.ShadowDistance;
            Vector3       up         = Vector3(0.0f, 1.0f, 0.0f);
            if (glm::abs(glm::dot(up, lightDir)) > 0.95f) { up = Vector3(0.0f, 0.0f, 1.0f); }
            Matrix4 lightView = glm::lookAt(lightPos, focusPoint, up);
            Matrix4 lightProj = glm::ortho(-sceneData_.ShadowOrthoSize, sceneData_.ShadowOrthoSize,
                                           -sceneData_.ShadowOrthoSize, sceneData_.ShadowOrthoSize,
                                           0.1f, sceneData_.ShadowDistance * 2.0f);
            sceneData_.LightSpaceMatrix = lightProj * lightView;
        } else {
            sceneData_.LightSpaceMatrix = Matrix4(1.0f);
        }
    }

    ResetStats();
}

void SceneRenderer::EndScene() {
    SE_LOG_INFO("EndScene: Starting (ShadowsEnabled={})", sceneData_.ShadowsEnabled);
    if (sceneData_.ShadowsEnabled) { 
        SE_LOG_INFO("EndScene: Running shadow pass");
        RenderShadowPass(); 
        SE_LOG_INFO("EndScene: Shadow pass complete");
    }
    SE_LOG_INFO("EndScene: Running scene pass ({} submissions, {} instanced)", 
                sceneData_.Submissions.size(), instancedSubmissions_.size());
    RenderScenePass();
    SE_LOG_INFO("EndScene: Complete");
}

void SceneRenderer::Submit(const std::shared_ptr<VertexArray>& vertexArray,
                           const std::shared_ptr<Material>& material, const Matrix4& transform,
                           bool castsShadows, bool receiveShadows, float boundingRadius) {
    Submission submission;
    submission.vertex_array   = vertexArray;
    submission.material       = material;
    submission.Transform      = transform;
    submission.CastsShadows   = castsShadows;
    submission.ReceiveShadows = receiveShadows;

    // Extract position from transform
    submission.Center = Vector3(transform[3]);

    // Create stable ObjectId based on position hash (for occlusion query tracking)
    // This ensures the same object gets the same ID across frames
    auto hashFloat      = [](float f) -> uint32_t { return *reinterpret_cast<uint32_t*>(&f); };
    submission.ObjectId = hashFloat(submission.Center.x) ^ (hashFloat(submission.Center.y) << 8) ^
                          (hashFloat(submission.Center.z) << 16);
    if (submission.ObjectId == 0) submission.ObjectId = 1;  // 0 is reserved

    // Calculate bounding radius considering scale
    float scaleX   = glm::length(Vector3(transform[0]));
    float scaleY   = glm::length(Vector3(transform[1]));
    float scaleZ   = glm::length(Vector3(transform[2]));
    float maxScale = glm::max(glm::max(scaleX, scaleY), scaleZ);

    // For a unit cube, bounding sphere radius is sqrt(3)/2 ≈ 0.866
    submission.BoundingRadius = boundingRadius * maxScale;

    sceneData_.Submissions.emplace_back(std::move(submission));
}

void SceneRenderer::SubmitInstanced(const std::shared_ptr<InstancedMesh>& instancedMesh,
                                    const std::shared_ptr<Material>& material, bool castsShadows,
                                    bool receiveShadows) {
    if (!instancedMesh || !material) {
        SE_LOG_WARN("SubmitInstanced called with null instancedMesh or material");
        return;
    }

    InstancedSubmission submission;
    submission.instancedMesh  = instancedMesh;
    submission.material       = material;
    submission.castsShadows   = castsShadows;
    submission.receiveShadows = receiveShadows;
    instancedSubmissions_.emplace_back(std::move(submission));
}

void SceneRenderer::SetOcclusionCullingEnabled(bool enabled) {
    occlusionCullingEnabled_ = enabled;
    occlusionCuller_.SetEnabled(enabled);
}

void SceneRenderer::SetDirectionalLight(const DirectionalLightData& light) {
    sceneData_.directional_light        = light;
    sceneData_.directional_light.Active = true;

    if (glm::length(sceneData_.directional_light.Direction) <= 0.0f) {
        sceneData_.directional_light.Direction = Vector3(0.0f, -1.0f, 0.0f);
    } else {
        sceneData_.directional_light.Direction =
            glm::normalize(sceneData_.directional_light.Direction);
    }

    sceneData_.directional_light.Intensity = glm::max(light.Intensity, 0.0f);
}

void SceneRenderer::ClearDirectionalLight() {
    sceneData_.directional_light = DirectionalLightData{};
    sceneData_.ShadowsEnabled    = false;
    sceneData_.LightSpaceMatrix  = Matrix4(1.0f);
}

SceneRenderer::DirectionalLightData SceneRenderer::GetDirectionalLight() const {
    return sceneData_.directional_light;
}

void SceneRenderer::SetShadowMapSize(int width, int height) {
    if (width <= 0 || height <= 0) {
        SE_LOG_WARN("Invalid shadow map size: {}x{}, using default 1024x1024", width, height);
        width  = 1024;
        height = 1024;
    }

    if (sceneData_.ShadowMapSize.x != width || sceneData_.ShadowMapSize.y != height) {
        sceneData_.ShadowMapSize = glm::ivec2(width, height);
        if (initialized_) {
            DestroyShadowResources();
            InitializeShadowResources();
        }
    }
}

void SceneRenderer::SetShadowDistance(float distance) {
    sceneData_.ShadowDistance = glm::max(distance, 1.0f);
}

void SceneRenderer::SetShadowOrthoSize(float size) {
    sceneData_.ShadowOrthoSize = glm::max(size, 1.0f);
}

void SceneRenderer::SetAmbientStrength(float strength) {
    sceneData_.AmbientStrength = glm::clamp(strength, 0.0f, 1.0f);
}

void SceneRenderer::SetAOStrength(float strength) {
    sceneData_.AOStrength = glm::clamp(strength, 0.0f, 1.0f);
}

void SceneRenderer::SetAORadius(float radius) {
    sceneData_.AORadius = glm::max(radius, 0.1f);
}

void SceneRenderer::InitializeShadowResources() {
    SE_LOG_INFO("Creating shadow resources ({}x{})", sceneData_.ShadowMapSize.x,
                sceneData_.ShadowMapSize.y);

    sceneData_.ShadowShader = std::make_shared<Shader>(kShadowVertexSource, kShadowFragmentSource);
    if (!sceneData_.ShadowShader || sceneData_.ShadowShader->getID() == 0) {
        SE_LOG_ERROR("Failed to create shadow shader");
        return;
    }

    // Create instanced shadow shader (uses same fragment, different vertex for instance buffer)
    sceneData_.InstancedShadowShader =
        std::make_shared<Shader>(kInstancedShadowVertexSource, kShadowFragmentSource);
    if (!sceneData_.InstancedShadowShader || sceneData_.InstancedShadowShader->getID() == 0) {
        SE_LOG_ERROR("Failed to create instanced shadow shader");
        return;
    }
    SE_LOG_INFO("Created instanced shadow shader successfully");

    glGenFramebuffers(1, &sceneData_.ShadowFramebuffer);
    if (sceneData_.ShadowFramebuffer == 0) {
        SE_LOG_ERROR("Failed to generate shadow framebuffer");
        return;
    }

    glGenTextures(1, &sceneData_.ShadowDepthTexture);
    if (sceneData_.ShadowDepthTexture == 0) {
        SE_LOG_ERROR("Failed to generate shadow depth texture");
        glDeleteFramebuffers(1, &sceneData_.ShadowFramebuffer);
        sceneData_.ShadowFramebuffer = 0;
        return;
    }

    glBindTexture(GL_TEXTURE_2D, sceneData_.ShadowDepthTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, sceneData_.ShadowMapSize.x,
                 sceneData_.ShadowMapSize.y, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

    GLenum texError = glGetError();
    if (texError != GL_NO_ERROR) {
        SE_LOG_ERROR("GL error creating shadow depth texture: 0x{:X}", texError);
    }

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    const float borderColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

    glBindFramebuffer(GL_FRAMEBUFFER, sceneData_.ShadowFramebuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D,
                           sceneData_.ShadowDepthTexture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);

    GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        SE_LOG_ERROR("Shadow framebuffer incomplete, status: 0x{:X}", status);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        DestroyShadowResources();
        return;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    SE_LOG_INFO("Shadow resources created successfully");
}

void SceneRenderer::DestroyShadowResources() {
    if (sceneData_.ShadowDepthTexture) {
        glDeleteTextures(1, &sceneData_.ShadowDepthTexture);
        sceneData_.ShadowDepthTexture = 0;
    }
    if (sceneData_.ShadowFramebuffer) {
        glDeleteFramebuffers(1, &sceneData_.ShadowFramebuffer);
        sceneData_.ShadowFramebuffer = 0;
    }
    sceneData_.ShadowShader.reset();
    sceneData_.InstancedShadowShader.reset();
}

void SceneRenderer::RenderShadowPass() {
    if (sceneData_.Submissions.empty() && instancedSubmissions_.empty()) return;
    if (!sceneData_.ShadowShader || !sceneData_.ShadowFramebuffer) return;

    // Save current framebuffer and viewport
    GLint previousFramebuffer = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);
    GLint previousViewport[4];
    glGetIntegerv(GL_VIEWPORT, previousViewport);

    glViewport(0, 0, sceneData_.ShadowMapSize.x, sceneData_.ShadowMapSize.y);
    glBindFramebuffer(GL_FRAMEBUFFER, sceneData_.ShadowFramebuffer);
    glClear(GL_DEPTH_BUFFER_BIT);

    GLboolean wasCullEnabled       = glIsEnabled(GL_CULL_FACE);
    GLint     previousCullFaceMode = GL_BACK;
    if (wasCullEnabled) glGetIntegerv(GL_CULL_FACE_MODE, &previousCullFaceMode);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    sceneData_.ShadowShader->bind();
    sceneData_.ShadowShader->setMat4("uLightSpaceMatrix", sceneData_.LightSpaceMatrix);

    for (const auto& submission : sceneData_.Submissions) {
        if (!submission.CastsShadows) continue;
        if (!submission.vertex_array) continue;

        sceneData_.ShadowShader->setMat4("uModel", submission.Transform);
        RenderCommand::DrawIndexed(submission.vertex_array.get());
    }

    // Render shadows for instanced submissions using instanced shadow shader
    if (!instancedSubmissions_.empty() && sceneData_.InstancedShadowShader) {
        sceneData_.InstancedShadowShader->bind();
        sceneData_.InstancedShadowShader->setMat4("uLightSpaceMatrix", sceneData_.LightSpaceMatrix);

        for (const auto& instanced : instancedSubmissions_) {
            if (!instanced.castsShadows) continue;
            if (!instanced.instancedMesh) continue;

            auto va = instanced.instancedMesh->GetVertexArray();
            if (!va) continue;

            uint32_t instanceCount = instanced.instancedMesh->GetInstanceCount();
            if (instanceCount == 0) continue;

            // Draw all instances in a single call - shader reads transform from instance buffer
            RenderCommand::DrawIndexedInstanced(va.get(), instanceCount);
        }
    }

    glCullFace(previousCullFaceMode);
    if (!wasCullEnabled) glDisable(GL_CULL_FACE);

    // Restore previous framebuffer and viewport (not just 0!)
    glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
}

void SceneRenderer::RenderScenePass() {
    SE_LOG_INFO("RenderScenePass: Starting");
    SE_LOG_INFO("RenderScenePass: Calling glActiveTexture");
    glActiveTexture(GL_TEXTURE0);
    SE_LOG_INFO("RenderScenePass: glActiveTexture done");
    if (sceneData_.ShadowsEnabled && sceneData_.ShadowDepthTexture) {
        SE_LOG_INFO("RenderScenePass: Binding shadow texture");
        glBindTexture(GL_TEXTURE_2D, sceneData_.ShadowDepthTexture);
    } else {
        SE_LOG_INFO("RenderScenePass: Binding null texture");
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    SE_LOG_INFO("RenderScenePass: Texture binding done");

    SE_LOG_INFO("RenderScenePass: Setting up occlusion culler");
    SE_LOG_INFO("RenderScenePass: Calling SetViewProjection");
    occlusionCuller_.SetViewProjection(sceneData_.view_projection_matrix);
    SE_LOG_INFO("RenderScenePass: Calling BeginFrame");
    occlusionCuller_.BeginFrame();
    SE_LOG_INFO("RenderScenePass: Calling ResetStats");
    occlusionCuller_.ResetStats();
    SE_LOG_INFO("RenderScenePass: Occlusion culler setup complete");

    // Separate into occluders (large) and occludees (small)
    std::vector<const Submission*> occluders;
    std::vector<const Submission*> occludees;
    const float                    kOccluderThreshold = 5.0f;

    SE_LOG_INFO("RenderScenePass: Sorting {} submissions", sceneData_.Submissions.size());
    for (const auto& submission : sceneData_.Submissions) {
        if (!submission.vertex_array || !submission.material) continue;

        stats_.TotalObjects++;

        // Frustum culling first
        if (frustumCullingEnabled_) {
            // SE_LOG_INFO("RenderScenePass: Checking frustum for object {}", submission.ObjectId);
            if (!occlusionCuller_.IsSphereVisible(submission.Center, submission.BoundingRadius)) {
                stats_.FrustumCulled++;
                continue;
            }
        }

        if (submission.BoundingRadius > kOccluderThreshold) {
            occluders.push_back(&submission);
        } else {
            occludees.push_back(&submission);
        }
    }
    SE_LOG_INFO("RenderScenePass: Submissions sorted: {} occluders, {} occludees", occluders.size(), occludees.size());

    // Lambda to render a single object
    auto renderObject = [this](const Submission& submission) {
        SE_LOG_INFO("renderObject: Binding material for object {}", submission.ObjectId);
        if (!submission.material) {
            SE_LOG_ERROR("renderObject: Material is null for object {}", submission.ObjectId);
            return;
        }
        submission.material->Bind();
        
        auto shader = submission.material->GetShader();
        if (!shader) {
            SE_LOG_ERROR("renderObject: Shader is null for object {}", submission.ObjectId);
            return;
        }

        SE_LOG_INFO("renderObject: Setting uniforms for object {}", submission.ObjectId);
        shader->setMat4("uView", sceneData_.ViewMatrix);
        shader->setMat4("uProj", sceneData_.ProjectionMatrix);
        shader->setMat4("uModel", submission.Transform);
        shader->setVec3("uLightDirection", -sceneData_.directional_light.Direction);
        shader->setVec3("uLightColor", sceneData_.directional_light.Color);
        shader->setFloat("uLightIntensity", sceneData_.directional_light.Active
                                                ? sceneData_.directional_light.Intensity
                                                : 0.0f);
        shader->setFloat("uAmbientStrength", sceneData_.AmbientStrength);
        shader->setMat4("uLightSpaceMatrix", sceneData_.LightSpaceMatrix);
        shader->setInt("uShadowMap", 0);
        shader->setFloat("uReceiveShadows", submission.ReceiveShadows ? 1.0f : 0.0f);
        shader->setFloat(
            "uShadowsEnabled",
            sceneData_.ShadowsEnabled && sceneData_.directional_light.Active ? 1.0f : 0.0f);
        shader->setFloat("uAOStrength", sceneData_.AOStrength);
        shader->setFloat("uAORadius", sceneData_.AORadius);

        SE_LOG_INFO("renderObject: Drawing object {}", submission.ObjectId);
        if (!submission.vertex_array) {
            SE_LOG_ERROR("renderObject: VertexArray is null for object {}", submission.ObjectId);
            return;
        }
        RenderCommand::DrawIndexed(submission.vertex_array.get());

        stats_.VisibleObjects++;
        stats_.DrawCalls++;
        stats_.TriangleCount += submission.vertex_array->GetIndexBuffer()->GetCount() / 3;
        SE_LOG_INFO("renderObject: Object {} done", submission.ObjectId);
    };

    // PHASE 1: Render all OCCLUDERS first to fill depth buffer
    SE_LOG_INFO("RenderScenePass: PHASE 1 - Rendering {} occluders", occluders.size());
    for (const auto* submission : occluders) { renderObject(*submission); }
    SE_LOG_INFO("RenderScenePass: PHASE 1 complete");

    // PHASE 2: Test occludee bounding boxes against depth buffer filled by occluders
    if (occlusionCullingEnabled_ && occlusionCuller_.IsEnabled()) {
        SE_LOG_INFO("RenderScenePass: PHASE 2 - Occusion testing {} occludees", occludees.size());
        for (const auto* submission : occludees) {
            occlusionCuller_.BeginQuery(submission->ObjectId);
            occlusionCuller_.RenderBoundingBox(submission->Center,
                                               Vector3(submission->BoundingRadius));
            occlusionCuller_.EndQuery();
        }
        SE_LOG_INFO("RenderScenePass: PHASE 2 complete");

        // Collect results immediately (blocking) to use this frame
        SE_LOG_INFO("RenderScenePass: PHASE 3 - Collecting results");
        occlusionCuller_.CollectResults();
        SE_LOG_INFO("RenderScenePass: PHASE 3 complete");
    }

    // PHASE 3: Render occludees that passed the visibility test
    for (const auto* submission : occludees) {
        if (occlusionCullingEnabled_ && occlusionCuller_.IsEnabled()) {
            if (!occlusionCuller_.WasVisibleLastFrame(submission->ObjectId)) {
                stats_.OcclusionCulled++;
                continue;  // Skip - occluded by occluders
            }
        }
        renderObject(*submission);
    }

    // PHASE 4: Render instanced batches (no culling - already handled by RenderSystem)
    SE_LOG_INFO("RenderScenePass: PHASE 4 - {} instanced batches", instancedSubmissions_.size());
    int batchNum = 0;
    for (const auto& instanced : instancedSubmissions_) {
        if (!instanced.instancedMesh || !instanced.material) continue;

        uint32_t instanceCount = instanced.instancedMesh->GetInstanceCount();
        if (instanceCount == 0) continue;

        SE_LOG_INFO("RenderScenePass: Batch {} with {} instances", batchNum, instanceCount);

        instanced.material->Bind();
        auto shader = instanced.material->GetShader();
        if (!shader) {
            batchNum++;
            continue;
        }
        SE_LOG_INFO("RenderScenePass: Shader bound for batch {}", batchNum);

        // Set uniforms (same as normal rendering, but no uModel - that comes from instance buffer)
        shader->setMat4("uView", sceneData_.ViewMatrix);
        shader->setMat4("uProj", sceneData_.ProjectionMatrix);
        shader->setVec3("uLightDirection", -sceneData_.directional_light.Direction);
        shader->setVec3("uLightColor", sceneData_.directional_light.Color);
        shader->setFloat("uLightIntensity", sceneData_.directional_light.Active
                                                ? sceneData_.directional_light.Intensity
                                                : 0.0f);
        shader->setFloat("uAmbientStrength", sceneData_.AmbientStrength);
        shader->setMat4("uLightSpaceMatrix", sceneData_.LightSpaceMatrix);
        shader->setInt("uShadowMap", 0);
        shader->setFloat("uReceiveShadows", instanced.receiveShadows ? 1.0f : 0.0f);
        shader->setFloat(
            "uShadowsEnabled",
            sceneData_.ShadowsEnabled && sceneData_.directional_light.Active ? 1.0f : 0.0f);
        shader->setFloat("uAOStrength", sceneData_.AOStrength);
        shader->setFloat("uAORadius", sceneData_.AORadius);

        // Single draw call for all instances in this batch
        instanced.instancedMesh->DrawWithoutMaterial();

        stats_.InstancedBatches++;
        stats_.InstancedObjects += instanceCount;
        stats_.DrawCalls++;

        const auto& indexBuffer = instanced.instancedMesh->GetVertexArray()->GetIndexBuffer();
        if (indexBuffer) { stats_.TriangleCount += (indexBuffer->GetCount() / 3) * instanceCount; }
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

}  // namespace se
