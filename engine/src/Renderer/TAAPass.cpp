#include "engine/renderer/TAAPass.h"
#include "engine/Shader.h"
#include "engine/Log.h"

#include <glad/glad.h>

namespace se {

namespace {
    float halton(int index, int base) {
        float f = 1.0f;
        float r = 0.0f;
        int i = index;
        while (i > 0) {
            f /= float(base);
            r += f * float(i % base);
            i /= base;
        }
        return r;
    }
}

TAAPass::TAAPass() = default;

TAAPass::~TAAPass() {
    Shutdown();
}

bool TAAPass::Initialize(int width, int height) {
    if (initialized_) return true;
    
    SE_LOG_INFO("TAAPass: Initializing ({}x{})", width, height);
    
    width_ = width;
    height_ = height;
    
    shader_ = std::make_shared<Shader>(
        "assets/shaders/postprocess/fullscreen_quad.vert",
        "assets/shaders/postprocess/taa.frag"
    );
    
    if (!shader_ || shader_->getID() == 0) {
        SE_LOG_ERROR("TAAPass: Failed to load shader");
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
    SE_LOG_INFO("TAAPass: Initialized successfully");
    return true;
}

void TAAPass::Shutdown() {
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
    
    SE_LOG_INFO("TAAPass: Shutdown");
}

void TAAPass::Resize(int width, int height) {
    if (width_ == width && height_ == height) return;
    
    width_ = width;
    height_ = height;
    DestroyResources();
    CreateResources(width, height);
}

void TAAPass::CreateResources(int width, int height) {
    for (int i = 0; i < 2; i++) {
        glGenFramebuffers(1, &fbo_[i]);
        glGenTextures(1, &historyTexture_[i]);
        
        glBindTexture(GL_TEXTURE_2D, historyTexture_[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, historyTexture_[i], 0);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void TAAPass::DestroyResources() {
    for (int i = 0; i < 2; i++) {
        if (fbo_[i] != 0) {
            glDeleteFramebuffers(1, &fbo_[i]);
            fbo_[i] = 0;
        }
        if (historyTexture_[i] != 0) {
            glDeleteTextures(1, &historyTexture_[i]);
            historyTexture_[i] = 0;
        }
    }
}

glm::vec2 TAAPass::GetHaltonJitter(int index) const {
    return glm::vec2(
        halton(index + 1, 2) - 0.5f,
        halton(index + 1, 3) - 0.5f
    );
}

void TAAPass::AdvanceJitter() {
    jitterIndex_ = (jitterIndex_ + 1) % settings_.jitterSequenceLength;
    
    glm::vec2 jitter = GetHaltonJitter(jitterIndex_);
    currentJitter_ = jitter / glm::vec2(width_, height_);
}

void TAAPass::Apply(
    uint32_t currentFrame,
    uint32_t depthTexture,
    uint32_t motionVectors
) {
    if (!initialized_ || !settings_.enabled) return;
    
    int outputBuffer = 1 - currentBuffer_;
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_[outputBuffer]);
    
    glViewport(0, 0, width_, height_);
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    
    shader_->bind();
    
    shader_->setInt("uCurrentFrame", 0);
    shader_->setInt("uHistoryFrame", 1);
    shader_->setInt("uMotionVectors", 2);
    shader_->setInt("uDepthTexture", 3);
    
    shader_->setVec2("uResolution", glm::vec2(width_, height_));
    shader_->setVec2("uJitter", currentJitter_);
    shader_->setFloat("uBlendFactor", settings_.blendFactor);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, currentFrame);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, historyTexture_[currentBuffer_]);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, motionVectors);
    glActiveTexture(GL_TEXTURE3);
    glBindTexture(GL_TEXTURE_2D, depthTexture);
    
    RenderFullscreenQuad();
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glEnable(GL_DEPTH_TEST);
    
    currentBuffer_ = outputBuffer;
}

uint32_t TAAPass::GetResultTexture() const {
    return historyTexture_[currentBuffer_];
}

void TAAPass::RenderFullscreenQuad() {
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

} // namespace se
