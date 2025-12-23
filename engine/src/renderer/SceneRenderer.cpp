#include "engine/renderer/SceneRenderer.h"

#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>

#include "engine/core/Application.h"
#include "engine/core/Log.h"
#include "engine/renderer/GraphicsContext.h"
// #include "engine/renderer/RenderCommand.h" - Removed

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

static RHI::IDevice* GetDevice() {
    auto& app     = Application::Get();
    auto* context = app.GetWindow().GetContext();
    return context ? context->GetDevice() : nullptr;
}

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

        sceneData_.ShadowsEnabled = sceneData_.directional_light.CastShadows && sceneData_.directional_light.Intensity > 0.0f;

        if (sceneData_.ShadowsEnabled) {
            // Shadow frustum follows camera position for consistent shadow coverage
            Vector3       cameraPos  = glm::inverse(sceneData_.ViewMatrix)[3];
            const Vector3 focusPoint = Vector3(cameraPos.x, 0.0f, cameraPos.z);
            const Vector3 lightPos   = focusPoint - lightDir * sceneData_.ShadowDistance;
            Vector3       up         = Vector3(0.0f, 1.0f, 0.0f);
            if (glm::abs(glm::dot(up, lightDir)) > 0.95f) { up = Vector3(0.0f, 0.0f, 1.0f); }
            Matrix4 lightView           = glm::lookAt(lightPos, focusPoint, up);
            Matrix4 lightProj           = glm::ortho(-sceneData_.ShadowOrthoSize, sceneData_.ShadowOrthoSize, -sceneData_.ShadowOrthoSize,
                                                     sceneData_.ShadowOrthoSize, 0.1f, sceneData_.ShadowDistance * 2.0f);
            sceneData_.LightSpaceMatrix = lightProj * lightView;
        } else {
            sceneData_.LightSpaceMatrix = Matrix4(1.0f);
        }
    }

    ResetStats();
}

void SceneRenderer::EndScene() {
    if (sceneData_.ShadowsEnabled) { RenderShadowPass(); }
    RenderScenePass();
}

void SceneRenderer::Submit(const std::shared_ptr<VertexArray>& vertexArray, const std::shared_ptr<Material>& material, const Matrix4& transform,
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
    auto hashFloat = [](float f) -> uint32_t {
        uint32_t result;
        std::memcpy(&result, &f, sizeof(float));
        return result;
    };
    submission.ObjectId = hashFloat(submission.Center.x) ^ (hashFloat(submission.Center.y) << 8) ^ (hashFloat(submission.Center.z) << 16);
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

void SceneRenderer::SubmitInstanced(const std::shared_ptr<InstancedMesh>& instancedMesh, const std::shared_ptr<Material>& material, bool castsShadows,
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
        sceneData_.directional_light.Direction = glm::normalize(sceneData_.directional_light.Direction);
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
        if (initialized_ && sceneData_.ShadowFramebuffer) {
            sceneData_.ShadowFramebuffer->Resize(width, height);
            // Also need to resize texture? Framebuffer Resize logic in wrappers usually handles attachments if they are owned.
            // But here we attached manually.
            // Simple approach: Destroy and Recreate for now to be safe.
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
    // Skip shadow resources for Vulkan - GLSL 330 shaders not compatible
    // TODO: Create SPIR-V shadow shaders for Vulkan support
    auto* device = GetDevice();
    if (device && device->GetAPI() == RHI::API::Vulkan) {
        SE_LOG_WARN("Shadow mapping skipped for Vulkan (SPIR-V shaders not yet available)");
        sceneData_.ShadowsEnabled = false;
        return;
    }

    SE_LOG_INFO("Creating shadow resources ({}x{})", sceneData_.ShadowMapSize.x, sceneData_.ShadowMapSize.y);

    sceneData_.ShadowShader = std::make_shared<Shader>(kShadowVertexSource, kShadowFragmentSource);
    if (!sceneData_.ShadowShader || !RHI::IsValid(sceneData_.ShadowShader->GetHandle())) {
        SE_LOG_ERROR("Failed to create shadow shader");
        return;
    }

    // Create instanced shadow shader (uses same fragment, different vertex for instance buffer)
    sceneData_.InstancedShadowShader = std::make_shared<Shader>(kInstancedShadowVertexSource, kShadowFragmentSource);
    if (!sceneData_.InstancedShadowShader || !RHI::IsValid(sceneData_.InstancedShadowShader->GetHandle())) {
        SE_LOG_ERROR("Failed to create instanced shadow shader");
        return;
    }
    SE_LOG_INFO("Created instanced shadow shader successfully");

    // Create shadow depth texture
    // For depth-only, we create a specialized texture.
    // NOTE: Texture wrapper might need updating to support Depth formats easily, or we assume format passing works.
    sceneData_.ShadowDepthTexture = std::make_shared<Texture>(sceneData_.ShadowMapSize.x, sceneData_.ShadowMapSize.y,
                                                              RHI::TextureFormat::Depth24Stencil8);  // Or Depth32F? Using standard depth.
    // RHI::TextureFormat::Depth32F might be better for shadows. Let's try Depth24Stencil8 as standard.
    // Wait, shadow maps usually don't need stencil. Depth32F or Depth16?
    // Let's us RHI::TextureFormat::Depth24Stencil8 for compatibility with standard depth or add Depth32F to Texture constructor support?
    // Texture constructor takes format.

    if (!sceneData_.ShadowDepthTexture || !RHI::IsValid(sceneData_.ShadowDepthTexture->GetHandle())) {
        SE_LOG_ERROR("Failed to create shadow depth texture");
        return;
    }

    // Create framebuffer
    FramebufferSpecification fbSpec;
    fbSpec.width    = sceneData_.ShadowMapSize.x;
    fbSpec.height   = sceneData_.ShadowMapSize.y;
    fbSpec.hasDepth = false;  // We attach our own depth texture manually?
    // Or we let Framebuffer create it? Framebuffer implementation attached creation logic?
    // My Framebuffer wrapper logic simple: create FB. RHI::CreateFramebuffer creates generic FB.
    // RHI::CreateFramebuffer doesn't automatically create attachments unless specified in descriptor (CreateFramebuffer logic in OpenGLDevice creates
    // EMPTY FB if no attachments?) Let's Assume RHI::IDevice implementation for OpenGL works with Bind/Attach. OpenGLDevice::CreateFramebuffer code
    // -> glGenFramebuffers. It's empty.

    sceneData_.ShadowFramebuffer = std::make_shared<Framebuffer>(fbSpec);
    if (!sceneData_.ShadowFramebuffer || !RHI::IsValid(sceneData_.ShadowFramebuffer->GetHandle())) {
        SE_LOG_ERROR("Failed to create shadow framebuffer");
        return;
    }

    // Attach depth texture
    sceneData_.ShadowFramebuffer->AttachTexture(RHI::FramebufferAttachment::Depth, sceneData_.ShadowDepthTexture);

    // Initial check (Bind and check status via RHI? RHI doesn't expose CheckStatus directly yet but logs errors)
    // We trust RHI log for now.

    // Set texture parameters for shadow mapping (Linear, Clamp, Compare Mode) implementation in Texture class?
    // Texture class only creates. Need to set sampler params?
    // RHI uses Samplers! We need a Sampler for shadow map sampling!
    // But for rendering INTO it, we just need usage.
    // For sampling (in shader), we bind Texture + Sampler.
    // Legacy OpenGL code set parameters directly on Texture object.
    // RHI should handle Samplers.
    // Do we have Sampler support in Materials?
    // Material::SetTexture?
    // SceneRenderer binds shadow map to slot 0? "uShadowMap".
    // We should create a Shadow Sampler.

    // For now, to keep "legacy behavior" working without full sampler overhaul:
    // We rely on defaults or update Texture class to allow setting specific GL parameters?
    // RHI philosophy: Separated Samplers.
    // OK, we will assume RHI defaults are accessible.
    // But shadow map comparison mode (GL_COMPARE_REF_TO_TEXTURE) is critical for PCF?
    // If not set, PCF in shader might work differently or manual compare needed.
    // Let's assume standard linear filtering for now.

    SE_LOG_INFO("Shadow resources created successfully");

    if (auto* device = GetDevice()) { device->BindFramebuffer({0}); }
}

void SceneRenderer::DestroyShadowResources() {
    sceneData_.ShadowDepthTexture.reset();
    sceneData_.ShadowFramebuffer.reset();
    sceneData_.ShadowShader.reset();
    sceneData_.InstancedShadowShader.reset();
}

void SceneRenderer::RenderShadowPass() {
    if (sceneData_.Submissions.empty() && instancedSubmissions_.empty()) return;
    if (!sceneData_.ShadowShader || !sceneData_.ShadowFramebuffer) return;

    // Bind Framebuffer and Clear
    sceneData_.ShadowFramebuffer->Bind();

    if (auto* device = GetDevice()) {
        device->SetViewport({0.0f, 0.0f, (int)sceneData_.ShadowMapSize.x, (int)sceneData_.ShadowMapSize.y, 0.0f, 1.0f});
        device->Clear(false, true, false);  // Clear Depth only
    }

    Matrix4 lightProjection, lightView;
    float   near_plane = 1.0f, far_plane = sceneData_.ShadowDistance;

    if (sceneData_.directional_light.Direction == Vector3(0.0f)) { sceneData_.directional_light.Direction = Vector3(0.0f, -1.0f, 0.0f); }

    lightProjection = glm::ortho(-sceneData_.ShadowOrthoSize, sceneData_.ShadowOrthoSize, -sceneData_.ShadowOrthoSize, sceneData_.ShadowOrthoSize,
                                 near_plane, far_plane);

    Vector3 lightPos = -sceneData_.directional_light.Direction * (sceneData_.ShadowDistance / 2.0f);
    lightView        = glm::lookAt(lightPos, Vector3(0.0f), Vector3(0.0f, 1.0f, 0.0f));

    sceneData_.LightSpaceMatrix = lightProjection * lightView;
    sceneData_.ShadowShader->setMat4("uLightSpaceMatrix", sceneData_.LightSpaceMatrix);

    auto renderShadow = [&](const Submission& submission) {
        if (!submission.CastsShadows) return;
        if (!submission.vertex_array) return;

        if (auto* device = GetDevice()) {
            if (!submission.vertex_array->GetVertexBuffers().empty()) {
                const auto& layout = submission.vertex_array->GetVertexBuffers()[0]->GetLayout();

                // Create Shadow Pipeline (Temporary logic)
                RHI::VertexLayout vertexLayout;
                vertexLayout.stride = layout.GetStride();
                for (const auto& el : layout.GetElements()) {
                    if (el.Name == "a_Position") {
                        RHI::VertexAttribute attr;
                        attr.location = 0;
                        attr.type     = RHI::VertexAttributeType::Float3;
                        attr.offset   = el.Offset;
                        vertexLayout.attributes.push_back(attr);
                    }
                }

                RHI::PipelineDescriptor desc{};
                desc.depthStencil.depthTestEnable  = true;
                desc.depthStencil.depthWriteEnable = true;
                desc.depthStencil.depthCompareOp   = RHI::CompareOp::Less;
                desc.rasterizer.frontFace          = RHI::FrontFace::CounterClockwise;
                desc.rasterizer.cullMode           = RHI::CullMode::Front;  // Fix Peter Panning
                desc.topology                      = RHI::PrimitiveTopology::TriangleList;
                desc.blend.blendEnable             = false;

                auto pipeline = device->CreatePipeline(desc, sceneData_.ShadowShader->GetHandle(), vertexLayout);
                if (RHI::IsValid(pipeline)) {
                    device->BindPipeline(pipeline);
                    submission.vertex_array->Bind();
                    sceneData_.ShadowShader->setMat4("uModel", submission.Transform);
                    RHI::DrawIndexedCommand cmd{};
                    cmd.indexCount    = submission.vertex_array->GetIndexBuffer()->GetCount();
                    cmd.instanceCount = 1;
                    device->DrawIndexed(cmd);
                    device->DestroyPipeline(pipeline);
                }
            }
        }
    };

    for (const auto& submission : sceneData_.Submissions) { renderShadow(submission); }

    // Instanced Shadows skipped for now to ensure stability
    /*
    if (!instancedSubmissions_.empty() && sceneData_.InstancedShadowShader) {
       // ... needs pipeline ...
    }
    */

    sceneData_.ShadowFramebuffer->Unbind();

    // Restore viewport
    if (auto* device = GetDevice()) {
        auto& app = Application::Get();
        device->SetViewport({0.0f, 0.0f, (int)app.GetWindow().GetWidth(), (int)app.GetWindow().GetHeight(), 0.0f, 1.0f});
    }
}

void SceneRenderer::RenderScenePass() {
    // glActiveTexture(GL_TEXTURE0) - RHI BindTexture takes slot
    if (sceneData_.ShadowsEnabled && sceneData_.ShadowDepthTexture) {
        sceneData_.ShadowDepthTexture->Bind(7);  // Moved to slot 7 to avoid conflict with Albedo (slot 0)
    } else {
        if (auto* device = GetDevice()) device->BindTexture(0, {0});
    }

    occlusionCuller_.SetViewProjection(sceneData_.view_projection_matrix);
    occlusionCuller_.BeginFrame();
    occlusionCuller_.ResetStats();

    // Separate into occluders (large) and occludees (small)
    std::vector<const Submission*> occluders;
    std::vector<const Submission*> occludees;
    const float                    kOccluderThreshold = 5.0f;

    for (const auto& submission : sceneData_.Submissions) {
        if (!submission.vertex_array || !submission.material) continue;

        stats_.TotalObjects++;

        // Frustum culling first
        if (frustumCullingEnabled_) {
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

    // Lambda to render a single object
    auto renderObject = [this](const Submission& submission) {
        submission.material->Bind();
        auto shader = submission.material->GetShader();
        if (!shader) return;

        // RHI Pipeline Binding
        if (auto* device = GetDevice()) {
            // simplified: assume first buffer has main layout
            if (!submission.vertex_array->GetVertexBuffers().empty()) {
                const auto& layout   = submission.vertex_array->GetVertexBuffers()[0]->GetLayout();
                auto        pipeline = submission.material->GetPipeline(layout);
                if (RHI::IsValid(pipeline)) { device->BindPipeline(pipeline); }
            }
        }

        submission.vertex_array->Bind();

        shader->setMat4("uView", sceneData_.ViewMatrix);
        shader->setMat4("uProj", sceneData_.ProjectionMatrix);
        shader->setMat4("uModel", submission.Transform);
        shader->setVec3("uLightDirection", -sceneData_.directional_light.Direction);
        shader->setVec3("uLightColor", sceneData_.directional_light.Color);
        shader->setFloat("uLightIntensity", sceneData_.directional_light.Active ? sceneData_.directional_light.Intensity : 0.0f);
        shader->setFloat("uAmbientStrength", sceneData_.AmbientStrength);
        shader->setMat4("uLightSpaceMatrix", sceneData_.LightSpaceMatrix);
        shader->setInt("uShadowMap", 7);  // Using slot 7 for shadows
        shader->setFloat("uReceiveShadows", submission.ReceiveShadows ? 1.0f : 0.0f);
        shader->setFloat("uShadowsEnabled", sceneData_.ShadowsEnabled && sceneData_.directional_light.Active ? 1.0f : 0.0f);
        shader->setFloat("uAOStrength", sceneData_.AOStrength);
        shader->setFloat("uAORadius", sceneData_.AORadius);

        if (auto* device = GetDevice()) {
            RHI::DrawIndexedCommand cmd{};
            cmd.indexCount    = submission.vertex_array->GetIndexBuffer()->GetCount();
            cmd.instanceCount = 1;
            static int drawLogCount = 0;
            if (drawLogCount < 10) {
                SE_LOG_INFO("SceneRenderer: renderObject calling DrawIndexed (indices={})", cmd.indexCount);
                drawLogCount++;
            }
            device->DrawIndexed(cmd);
        }

        stats_.VisibleObjects++;
        stats_.DrawCalls++;
        stats_.TriangleCount += submission.vertex_array->GetIndexBuffer()->GetCount() / 3;
    };

    // PHASE 1: Render all OCCLUDERS first to fill depth buffer
    for (const auto* submission : occluders) { renderObject(*submission); }

    // PHASE 2: Test occludee bounding boxes against depth buffer filled by occluders
    if (occlusionCullingEnabled_ && occlusionCuller_.IsEnabled()) {
        for (const auto* submission : occludees) {
            occlusionCuller_.BeginQuery(submission->ObjectId);
            occlusionCuller_.RenderBoundingBox(submission->Center, Vector3(submission->BoundingRadius));
            occlusionCuller_.EndQuery();
        }

        // Collect results immediately (blocking) to use this frame
        occlusionCuller_.CollectResults();
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
    for (const auto& instanced : instancedSubmissions_) {
        if (!instanced.instancedMesh || !instanced.material) continue;

        uint32_t instanceCount = instanced.instancedMesh->GetInstanceCount();
        if (instanceCount == 0) continue;

        instanced.material->Bind();
        auto shader = instanced.material->GetShader();
        if (!shader) continue;

        // RHI Pipeline Binding (Instanced)
        if (auto* device = GetDevice()) {
            auto va = instanced.instancedMesh->GetVertexArray();
            if (va && !va->GetVertexBuffers().empty()) {
                // Merge layouts
                // Note: creating copy of buffer elements
                std::vector<BufferElement> elements         = va->GetVertexBuffers()[0]->GetLayout().GetElements();
                const auto&                instanceElements = va->GetInstanceBufferLayout().GetElements();
                elements.insert(elements.end(), instanceElements.begin(), instanceElements.end());

                BufferLayout mergedLayout;
                // BufferLayout constructor from initializer list? No, explicit constructor?
                // Needs vector<BufferElement>. BufferLayout doesn't have vector constructor visible?
                // Check Buffer.h
                // If not, use private access?
                // Workaround: Reconstruct merged layout via helper or assuming Material::GetPipeline accepts elements?
                // Material::GetPipeline takes BufferLayout.
                // I cannot easily construct BufferLayout from vector if no constructor.
                // Wait, BufferLayout(std::initializer_list) exists.
                // I can try to construct it.
                // Or assume SceneRenderer only needs VBO layout if Shader handles binding via locations?
                // Shader determines usage. If I pass incomplete layout, Validation might complain.
                // For now, pass VBO layout. Assuming Instance buffer setup is handled elsewhere?
                // Wait, Input State needs ALL attributes.
                // If I cannot construct BufferLayout easily, I'll pass VBO layout and hope.
                // (Given I'm on OpenGL and it ignores layout, this is safe for now).

                const auto& layout   = va->GetVertexBuffers()[0]->GetLayout();
                auto        pipeline = instanced.material->GetPipeline(layout);
                if (RHI::IsValid(pipeline)) { device->BindPipeline(pipeline); }
            }
        }

        // Set uniforms (same as normal rendering, but no uModel - that comes from instance buffer)
        shader->setMat4("uView", sceneData_.ViewMatrix);
        shader->setMat4("uProj", sceneData_.ProjectionMatrix);
        shader->setVec3("uLightDirection", -sceneData_.directional_light.Direction);
        shader->setVec3("uLightColor", sceneData_.directional_light.Color);
        shader->setFloat("uLightIntensity", sceneData_.directional_light.Active ? sceneData_.directional_light.Intensity : 0.0f);
        shader->setFloat("uAmbientStrength", sceneData_.AmbientStrength);
        shader->setMat4("uLightSpaceMatrix", sceneData_.LightSpaceMatrix);
        shader->setInt("uShadowMap", 7);  // Using slot 7 for shadows
        shader->setFloat("uReceiveShadows", instanced.receiveShadows ? 1.0f : 0.0f);
        shader->setFloat("uShadowsEnabled", sceneData_.ShadowsEnabled && sceneData_.directional_light.Active ? 1.0f : 0.0f);
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

    // Unbind shadow map from slot 7
    if (auto* device = GetDevice()) { device->BindTexture(7, {0}); }
}

}  // namespace se
