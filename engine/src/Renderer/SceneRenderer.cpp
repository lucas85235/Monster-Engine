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
}  // namespace

namespace se {

SceneRenderer::SceneRenderer() {}

SceneRenderer::~SceneRenderer() {
    if (initialized_) {
        Shutdown();
    }
}

void SceneRenderer::Init() {
    if (initialized_) {
        SE_LOG_WARN("SceneRenderer already initialized");
        return;
    }
    
    SE_LOG_INFO("Initializing SceneRenderer");
    InitializeShadowResources();
    initialized_ = true;
}

void SceneRenderer::Shutdown() {
    if (!initialized_) return;
    
    SE_LOG_INFO("Shutting down SceneRenderer");
    DestroyShadowResources();
    initialized_ = false;
}

void SceneRenderer::BeginScene(const Camera& camera, const Matrix4& projection) {
    sceneData_.ViewMatrix             = camera.getViewMatrix();
    sceneData_.ProjectionMatrix       = projection;
    sceneData_.view_projection_matrix = projection * sceneData_.ViewMatrix;
    sceneData_.Submissions.clear();

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
        lightDir                                = glm::normalize(lightDir);
        sceneData_.directional_light.Direction = lightDir;

        sceneData_.ShadowsEnabled = sceneData_.directional_light.CastShadows && 
                                    sceneData_.directional_light.Intensity > 0.0f;

        if (sceneData_.ShadowsEnabled) {
            // Shadow frustum follows camera position for consistent shadow coverage
            Vector3 cameraPos = glm::inverse(sceneData_.ViewMatrix)[3];
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
    if (sceneData_.ShadowsEnabled) { RenderShadowPass(); }
    RenderScenePass();
}

void SceneRenderer::Submit(const std::shared_ptr<VertexArray>& vertexArray, 
                           const std::shared_ptr<Material>& material, 
                           const Matrix4& transform,
                           bool castsShadows, bool receiveShadows) {
    Submission submission;
    submission.vertex_array   = vertexArray;
    submission.material       = material;
    submission.Transform      = transform;
    submission.CastsShadows   = castsShadows;
    submission.ReceiveShadows = receiveShadows;
    sceneData_.Submissions.emplace_back(std::move(submission));
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
        width = 1024;
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

void SceneRenderer::InitializeShadowResources() {
    SE_LOG_INFO("Creating shadow resources ({}x{})", 
                sceneData_.ShadowMapSize.x, sceneData_.ShadowMapSize.y);
    
    sceneData_.ShadowShader = std::make_shared<Shader>(kShadowVertexSource, kShadowFragmentSource);
    if (!sceneData_.ShadowShader || sceneData_.ShadowShader->getID() == 0) {
        SE_LOG_ERROR("Failed to create shadow shader");
        return;
    }

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
}

void SceneRenderer::RenderShadowPass() {
    if (sceneData_.Submissions.empty()) return;
    if (!sceneData_.ShadowShader || !sceneData_.ShadowFramebuffer) return;

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

    glCullFace(previousCullFaceMode);
    if (!wasCullEnabled) glDisable(GL_CULL_FACE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(previousViewport[0], previousViewport[1], previousViewport[2], previousViewport[3]);
}

void SceneRenderer::RenderScenePass() {
    glActiveTexture(GL_TEXTURE0);
    if (sceneData_.ShadowsEnabled && sceneData_.ShadowDepthTexture)
        glBindTexture(GL_TEXTURE_2D, sceneData_.ShadowDepthTexture);
    else
        glBindTexture(GL_TEXTURE_2D, 0);

    for (const auto& submission : sceneData_.Submissions) {
        if (!submission.vertex_array || !submission.material) continue;

        submission.material->Bind();
        auto shader = submission.material->GetShader();
        if (!shader) continue;

        shader->setMat4("uView", sceneData_.ViewMatrix);
        shader->setMat4("uProj", sceneData_.ProjectionMatrix);
        shader->setMat4("uModel", submission.Transform);
        shader->setVec3("uLightDirection", -sceneData_.directional_light.Direction);
        shader->setVec3("uLightColor", sceneData_.directional_light.Color);
        shader->setFloat("uLightIntensity", sceneData_.directional_light.Active ? 
                         sceneData_.directional_light.Intensity : 0.0f);
        shader->setFloat("uAmbientStrength", sceneData_.AmbientStrength);
        shader->setMat4("uLightSpaceMatrix", sceneData_.LightSpaceMatrix);
        shader->setInt("uShadowMap", 0);
        shader->setFloat("uReceiveShadows", submission.ReceiveShadows ? 1.0f : 0.0f);
        shader->setFloat("uShadowsEnabled", sceneData_.ShadowsEnabled && 
                         sceneData_.directional_light.Active ? 1.0f : 0.0f);

        RenderCommand::DrawIndexed(submission.vertex_array.get());

        stats_.DrawCalls++;
        stats_.TriangleCount += submission.vertex_array->GetIndexBuffer()->GetCount() / 3;
    }

    glBindTexture(GL_TEXTURE_2D, 0);
}

}  // namespace se
