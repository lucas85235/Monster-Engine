#include "engine/renderer/SSAOPass.h"
#include "engine/Log.h"

#include <imgui.h>
#include <random>

namespace se {

void SSAOPass::Init(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;

    SE_LOG_INFO("[SSAOPass] Initializing {}x{}", width, height);

    // Load shaders using file loading (CreateFromFiles handles #include)
    ssaoShader_ = Shader::CreateFromFiles(
        "assets/shaders/postprocess/fullscreen_quad.vert",
        "assets/shaders/postprocess/ssao.frag"
    );

    blurShader_ = Shader::CreateFromFiles(
        "assets/shaders/postprocess/fullscreen_quad.vert",
        "assets/shaders/postprocess/ssao_blur.frag"
    );

    GenerateKernel();
    CreateNoiseTexture();
    CreateFramebuffers();

    // Init quad
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

    SE_LOG_INFO("[SSAOPass] Initialized with kernel size {}", kernelSize_);
}

void SSAOPass::Shutdown() {
    DestroyFramebuffers();

    if (noiseTexture_) {
        glDeleteTextures(1, &noiseTexture_);
        noiseTexture_ = 0;
    }
    if (quadVAO_) {
        glDeleteVertexArrays(1, &quadVAO_);
        quadVAO_ = 0;
    }
    if (quadVBO_) {
        glDeleteBuffers(1, &quadVBO_);
        quadVBO_ = 0;
    }

    ssaoShader_.reset();
    blurShader_.reset();
}

void SSAOPass::Resize(uint32_t width, uint32_t height) {
    if (width == width_ && height == height_) return;

    width_ = width;
    height_ = height;

    DestroyFramebuffers();
    CreateFramebuffers();
}

void SSAOPass::Execute(GLuint inputTexture, GLuint outputFBO) {
    if (!ssaoShader_ || !blurShader_) return;

    // Save previous framebuffer state
    GLint previousFramebuffer;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &previousFramebuffer);

    // Pass 1: Generate SSAO
    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO_);
    glClear(GL_COLOR_BUFFER_BIT);

    ssaoShader_->bind();
    ssaoShader_->setInt("uDepthTexture", 0);
    ssaoShader_->setInt("uNormalTexture", 1);
    ssaoShader_->setInt("uNoiseTexture", 2);
    ssaoShader_->setMat4("uProjection", projection_);
    ssaoShader_->setMat4("uView", view_);
    ssaoShader_->setVec2("uNoiseScale", glm::vec2(width_ / 4.0f, height_ / 4.0f));
    ssaoShader_->setFloat("uRadius", radius_);
    ssaoShader_->setFloat("uBias", bias_);
    ssaoShader_->setFloat("uIntensity", intensity_);
    ssaoShader_->setInt("uKernelSize", kernelSize_);

    // Use cached uniform locations (gathered during Init)
    for (int i = 0; i < kernelSize_; ++i) {
        if (kernelLocations_[i] >= 0) {
            glUniform3fv(kernelLocations_[i], 1, &ssaoKernel_[i].x);
        }
    }

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, depthTexture_);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, normalTexture_);
    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, noiseTexture_);

    RenderQuad();

    // Pass 2: Blur
    glBindFramebuffer(GL_FRAMEBUFFER, blurFBO_);
    glClear(GL_COLOR_BUFFER_BIT);

    blurShader_->bind();
    blurShader_->setInt("uSSAOTexture", 0);
    blurShader_->setInt("uBlurSize", blurSize_);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, aoTexture_);

    RenderQuad();

    // Restore previous framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, previousFramebuffer);
}

void SSAOPass::RenderUI() {
    ImGui::SliderFloat("Radius", &radius_, 0.1f, 2.0f);
    ImGui::SliderFloat("Intensity", &intensity_, 0.5f, 5.0f);
    ImGui::SliderFloat("Bias", &bias_, 0.001f, 0.1f);
    ImGui::SliderInt("Blur Size", &blurSize_, 1, 8);
}

void SSAOPass::GenerateKernel() {
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;

    ssaoKernel_.clear();
    ssaoKernel_.reserve(kernelSize_);

    for (int i = 0; i < kernelSize_; ++i) {
        glm::vec3 sample(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator)
        );
        sample = glm::normalize(sample);
        sample *= randomFloats(generator);
        
        float scale = float(i) / float(kernelSize_);
        scale = 0.1f + scale * scale * (1.0f - 0.1f);  // lerp
        sample *= scale;
        
        ssaoKernel_.push_back(sample);
    }
    
    // Cache uniform locations to avoid string formatting in Execute()
    kernelLocations_.clear();
    kernelLocations_.reserve(kernelSize_);
    if (ssaoShader_) {
        for (int i = 0; i < kernelSize_; ++i) {
            char uniformName[32];
            snprintf(uniformName, sizeof(uniformName), "uSamples[%d]", i);
            kernelLocations_.push_back(ssaoShader_->getUniformLocation(uniformName));
        }
    }
}

void SSAOPass::CreateNoiseTexture() {
    std::uniform_real_distribution<float> randomFloats(0.0f, 1.0f);
    std::default_random_engine generator;

    std::vector<glm::vec3> ssaoNoise;
    for (int i = 0; i < 16; ++i) {
        glm::vec3 noise(
            randomFloats(generator) * 2.0f - 1.0f,
            randomFloats(generator) * 2.0f - 1.0f,
            0.0f
        );
        ssaoNoise.push_back(noise);
    }

    glGenTextures(1, &noiseTexture_);
    glBindTexture(GL_TEXTURE_2D, noiseTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, 4, 4, 0, GL_RGB, GL_FLOAT, ssaoNoise.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
}

void SSAOPass::CreateFramebuffers() {
    // SSAO FBO
    glGenFramebuffers(1, &ssaoFBO_);
    glGenTextures(1, &aoTexture_);

    glBindTexture(GL_TEXTURE_2D, aoTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width_, height_, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, ssaoFBO_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, aoTexture_, 0);

    // Blur FBO
    glGenFramebuffers(1, &blurFBO_);
    glGenTextures(1, &blurredAOTexture_);

    glBindTexture(GL_TEXTURE_2D, blurredAOTexture_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R16F, width_, height_, 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    glBindFramebuffer(GL_FRAMEBUFFER, blurFBO_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, blurredAOTexture_, 0);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SSAOPass::DestroyFramebuffers() {
    if (ssaoFBO_) {
        glDeleteFramebuffers(1, &ssaoFBO_);
        ssaoFBO_ = 0;
    }
    if (aoTexture_) {
        glDeleteTextures(1, &aoTexture_);
        aoTexture_ = 0;
    }
    if (blurFBO_) {
        glDeleteFramebuffers(1, &blurFBO_);
        blurFBO_ = 0;
    }
    if (blurredAOTexture_) {
        glDeleteTextures(1, &blurredAOTexture_);
        blurredAOTexture_ = 0;
    }
}

void SSAOPass::RenderQuad() {
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

}  // namespace se
