#include "engine/renderer/BloomPass.h"
#include "engine/Log.h"

#include <imgui.h>

namespace se {

void BloomPass::Init(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;

    SE_LOG_INFO("[BloomPass] Initializing {}x{}", width, height);

    thresholdShader_ = Shader::CreateFromFiles(
        "assets/shaders/postprocess/fullscreen_quad.vert",
        "assets/shaders/postprocess/bloom_threshold.frag"
    );
    downsampleShader_ = Shader::CreateFromFiles(
        "assets/shaders/postprocess/fullscreen_quad.vert",
        "assets/shaders/postprocess/bloom_downsample.frag"
    );
    upsampleShader_ = Shader::CreateFromFiles(
        "assets/shaders/postprocess/fullscreen_quad.vert",
        "assets/shaders/postprocess/bloom_upsample.frag"
    );
    compositeShader_ = Shader::CreateFromFiles(
        "assets/shaders/postprocess/fullscreen_quad.vert",
        "assets/shaders/postprocess/bloom_composite.frag"
    );

    CreateMipChain();

    float quadVertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
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

    SE_LOG_INFO("[BloomPass] Created {} mip levels", MIP_COUNT);
}

void BloomPass::Shutdown() {
    DestroyMipChain();

    if (quadVAO_) {
        glDeleteVertexArrays(1, &quadVAO_);
        quadVAO_ = 0;
    }
    if (quadVBO_) {
        glDeleteBuffers(1, &quadVBO_);
        quadVBO_ = 0;
    }

    thresholdShader_.reset();
    downsampleShader_.reset();
    upsampleShader_.reset();
    compositeShader_.reset();
}

void BloomPass::Resize(uint32_t width, uint32_t height) {
    if (width == width_ && height == height_) return;

    width_ = width;
    height_ = height;

    DestroyMipChain();
    CreateMipChain();
}

void BloomPass::Execute(GLuint inputTexture, GLuint outputFBO) {
    if (mipChain_.empty()) return;

    glDisable(GL_DEPTH_TEST);

    // 1. Threshold + first downsample
    glBindFramebuffer(GL_FRAMEBUFFER, mipChain_[0].fbo);
    glViewport(0, 0, mipChain_[0].width, mipChain_[0].height);
    
    thresholdShader_->bind();
    thresholdShader_->setInt("uTexture", 0);
    thresholdShader_->setFloat("uThreshold", threshold_);
    thresholdShader_->setFloat("uSoftKnee", softKnee_);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    RenderQuad();

    // 2. Downsample chain
    downsampleShader_->bind();
    downsampleShader_->setInt("uTexture", 0);
    
    for (size_t i = 1; i < mipChain_.size(); ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, mipChain_[i].fbo);
        glViewport(0, 0, mipChain_[i].width, mipChain_[i].height);
        
        downsampleShader_->setVec2("uTexelSize", glm::vec2(
            1.0f / mipChain_[i-1].width,
            1.0f / mipChain_[i-1].height
        ));
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mipChain_[i-1].texture);
        RenderQuad();
    }

    // 3. Upsample chain with additive blend
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);
    
    upsampleShader_->bind();
    upsampleShader_->setInt("uTexture", 0);
    upsampleShader_->setFloat("uFilterRadius", filterRadius_);
    
    for (int i = static_cast<int>(mipChain_.size()) - 2; i >= 0; --i) {
        glBindFramebuffer(GL_FRAMEBUFFER, mipChain_[i].fbo);
        glViewport(0, 0, mipChain_[i].width, mipChain_[i].height);
        
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, mipChain_[i+1].texture);
        RenderQuad();
    }
    
    glDisable(GL_BLEND);

    // 4. Composite bloom with original
    glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
    glViewport(0, 0, width_, height_);
    
    compositeShader_->bind();
    compositeShader_->setInt("uSceneTexture", 0);
    compositeShader_->setInt("uBloomTexture", 1);
    compositeShader_->setFloat("uIntensity", intensity_);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, mipChain_[0].texture);
    RenderQuad();

    glEnable(GL_DEPTH_TEST);
}

void BloomPass::RenderUI() {
    ImGui::SliderFloat("Threshold", &threshold_, 0.0f, 5.0f);
    ImGui::SliderFloat("Soft Knee", &softKnee_, 0.0f, 1.0f);
    ImGui::SliderFloat("Intensity", &intensity_, 0.0f, 2.0f);
    ImGui::SliderFloat("Filter Radius", &filterRadius_, 0.001f, 0.02f);
}

void BloomPass::CreateMipChain() {
    mipChain_.resize(MIP_COUNT);

    uint32_t mipWidth = width_ / 2;
    uint32_t mipHeight = height_ / 2;

    for (int i = 0; i < MIP_COUNT; ++i) {
        mipChain_[i].width = std::max(mipWidth, 1u);
        mipChain_[i].height = std::max(mipHeight, 1u);

        glGenFramebuffers(1, &mipChain_[i].fbo);
        glGenTextures(1, &mipChain_[i].texture);

        glBindTexture(GL_TEXTURE_2D, mipChain_[i].texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_R11F_G11F_B10F, mipChain_[i].width, mipChain_[i].height, 0, GL_RGB, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, mipChain_[i].fbo);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mipChain_[i].texture, 0);

        mipWidth /= 2;
        mipHeight /= 2;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void BloomPass::DestroyMipChain() {
    for (auto& mip : mipChain_) {
        if (mip.fbo) glDeleteFramebuffers(1, &mip.fbo);
        if (mip.texture) glDeleteTextures(1, &mip.texture);
    }
    mipChain_.clear();
}

void BloomPass::RenderQuad() {
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

}  // namespace se
