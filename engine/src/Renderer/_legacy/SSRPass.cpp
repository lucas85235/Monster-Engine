#include "engine/renderer/SSRPass.h"
#include "engine/Shader.h"
#include "engine/Log.h"

#include <glad/glad.h>

namespace se {

SSRPass::SSRPass() = default;

SSRPass::~SSRPass() {
    Shutdown();
}

bool SSRPass::Initialize(int width, int height) {
    if (initialized_) return true;
    
    SE_LOG_INFO("SSRPass: Initializing ({}x{})", width, height);
    
    width_ = width;
    height_ = height;
    
    shader_ = std::make_shared<Shader>(
        "assets/shaders/postprocess/fullscreen_quad.vert",
        "assets/shaders/postprocess/ssr.frag"
    );
    
    if (!shader_ || shader_->getID() == 0) {
        SE_LOG_ERROR("SSRPass: Failed to load shader");
        return false;
    }
    
    CreateResources(width, height);
    
    float quadVertices[] = {
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f
    };
    
    glGenVertexArrays(1, &quadVAO_);
    glGenBuffers(1, &quadVBO_);
    glBindVertexArray(quadVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
    
    initialized_ = true;
    SE_LOG_INFO("SSRPass: Initialized successfully");
    return true;
}

void SSRPass::Shutdown() {
    if (!initialized_) return;
    
    DestroyResources();
    
    if (quadVAO_ != 0) {
        glDeleteVertexArrays(1, &quadVAO_);
        quadVAO_ = 0;
    }
    if (quadVBO_ != 0) {
        glDeleteBuffers(1, &quadVBO_);
        quadVBO_ = 0;
    }
    
    shader_.reset();
    initialized_ = false;
    
    SE_LOG_INFO("SSRPass: Shutdown");
}

void SSRPass::Resize(int width, int height) {
    if (width_ == width && height_ == height) return;
    
    width_ = width;
    height_ = height;
    DestroyResources();
    CreateResources(width, height);
}

void SSRPass::CreateResources(int width, int height) {
    glGenFramebuffers(1, &fbo_);
    glGenTextures(1, &resultTexture_);
    
    glBindTexture(GL_TEXTURE_2D, resultTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resultTexture_, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSRPass::DestroyResources() {
    if (fbo_ != 0) {
        glDeleteFramebuffers(1, &fbo_);
        fbo_ = 0;
    }
    if (resultTexture_ != 0) {
        glDeleteTextures(1, &resultTexture_);
        resultTexture_ = 0;
    }
}

void SSRPass::Apply(
    uint32_t colorTexture,
    uint32_t depthTexture,
    uint32_t normalTexture,
    uint32_t roughnessTexture,
    const glm::mat4& viewMatrix,
    const glm::mat4& projectionMatrix
) {
    if (!initialized_ || !settings_.enabled) return;
    
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, width_, height_);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    
    shader_->bind();
    
    glm::mat4 invView = glm::inverse(viewMatrix);
    glm::mat4 invProj = glm::inverse(projectionMatrix);
    
    shader_->setInt("uColorTexture", 0);
    shader_->setInt("uDepthTexture", 1);
    shader_->setInt("uNormalTexture", 2);
    shader_->setInt("uRoughnessTexture", 3);
    
    shader_->setMat4("uViewMatrix", viewMatrix);
    shader_->setMat4("uProjectionMatrix", projectionMatrix);
    shader_->setMat4("uInvViewMatrix", invView);
    shader_->setMat4("uInvProjectionMatrix", invProj);
    
    shader_->setVec2("uResolution", glm::vec2(width_, height_));
    shader_->setFloat("uMaxDistance", settings_.maxDistance);
    shader_->setFloat("uThickness", settings_.thickness);
    shader_->setInt("uMaxSteps", settings_.maxSteps);
    shader_->setFloat("uRoughnessThreshold", settings_.roughnessThreshold);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, colorTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, normalTexture);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, roughnessTexture);
    
    RenderFullscreenQuad();
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_DEPTH_TEST);
}

void SSRPass::RenderFullscreenQuad() {
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

} // namespace se
