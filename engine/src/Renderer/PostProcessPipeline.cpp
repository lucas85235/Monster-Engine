#include "engine/renderer/PostProcessPipeline.h"
#include "engine/Log.h"

#include <imgui.h>

namespace se {

PostProcessPipeline::~PostProcessPipeline() {
    Shutdown();
}

void PostProcessPipeline::Init(uint32_t width, uint32_t height) {
    if (initialized_) {
        SE_LOG_WARN("[PostProcessPipeline] Already initialized");
        return;
    }

    width_ = width;
    height_ = height;

    SE_LOG_INFO("[PostProcessPipeline] Initializing with resolution {}x{}", width, height);

    CreateFramebuffers();
    InitFullscreenQuad();

    for (auto& pass : passes_) {
        pass->Init(width, height);
    }

    initialized_ = true;
    SE_LOG_INFO("[PostProcessPipeline] Initialized with {} passes", passes_.size());
}

void PostProcessPipeline::Shutdown() {
    if (!initialized_) return;

    SE_LOG_INFO("[PostProcessPipeline] Shutting down");

    for (auto& pass : passes_) {
        pass->Shutdown();
    }
    passes_.clear();

    DestroyFramebuffers();

    if (quadVAO_) {
        glDeleteVertexArrays(1, &quadVAO_);
        quadVAO_ = 0;
    }
    if (quadVBO_) {
        glDeleteBuffers(1, &quadVBO_);
        quadVBO_ = 0;
    }

    initialized_ = false;
}

void PostProcessPipeline::Resize(uint32_t width, uint32_t height) {
    if (width == width_ && height == height_) return;
    if (width == 0 || height == 0) return;

    SE_LOG_INFO("[PostProcessPipeline] Resizing to {}x{}", width, height);

    width_ = width;
    height_ = height;

    DestroyFramebuffers();
    CreateFramebuffers();

    for (auto& pass : passes_) {
        pass->Resize(width, height);
    }
}

void PostProcessPipeline::Execute(GLuint sceneTexture, GLuint sceneDepth, GLuint targetFBO,
                                   uint32_t targetWidth, uint32_t targetHeight) {
    if (!initialized_ || passes_.empty()) return;

    GLuint currentInput = sceneTexture;
    int pingPongIndex = 0;

    int enabledPassCount = 0;
    for (const auto& pass : passes_) {
        if (pass->IsEnabled()) enabledPassCount++;
    }

    if (enabledPassCount == 0) {
        currentOutput_ = -1;
        return;
    }

    int processedPasses = 0;
    for (size_t i = 0; i < passes_.size(); ++i) {
        auto& pass = passes_[i];
        if (!pass->IsEnabled()) continue;

        processedPasses++;
        bool isLastPass = (processedPasses == enabledPassCount);

        // Use targetFBO for the last pass (supports external framebuffers like editor viewport)
        GLuint outputFBO = isLastPass ? targetFBO : pingPongFBOs_[pingPongIndex];
        
        // Set correct viewport for output - use target dimensions for last pass if provided
        if (isLastPass && targetWidth > 0 && targetHeight > 0) {
            glViewport(0, 0, targetWidth, targetHeight);
        } else {
            glViewport(0, 0, width_, height_);
        }
        
        pass->Execute(currentInput, outputFBO);

        if (!isLastPass) {
            currentInput = pingPongTextures_[pingPongIndex];
            currentOutput_ = pingPongIndex;
            pingPongIndex = 1 - pingPongIndex;
        }
    }
}

void PostProcessPipeline::RenderUI() {
    if (ImGui::CollapsingHeader("Post-Process Pipeline")) {
        ImGui::Text("Resolution: %dx%d", width_, height_);
        ImGui::Text("Active Passes: %zu", passes_.size());
        ImGui::Separator();

        for (size_t i = 0; i < passes_.size(); ++i) {
            auto& pass = passes_[i];
            bool enabled = pass->IsEnabled();
            
            if (ImGui::Checkbox(pass->GetName(), &enabled)) {
                pass->SetEnabled(enabled);
            }

            if (enabled) {
                ImGui::Indent();
                pass->RenderUI();
                ImGui::Unindent();
            }
        }
    }
}

void PostProcessPipeline::CreateFramebuffers() {
    for (int i = 0; i < 2; ++i) {
        glGenFramebuffers(1, &pingPongFBOs_[i]);
        glGenTextures(1, &pingPongTextures_[i]);

        glBindTexture(GL_TEXTURE_2D, pingPongTextures_[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width_, height_, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, pingPongFBOs_[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, pingPongTextures_[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            SE_LOG_ERROR("[PostProcessPipeline] Ping-pong framebuffer {} incomplete", i);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void PostProcessPipeline::DestroyFramebuffers() {
    for (int i = 0; i < 2; ++i) {
        if (pingPongFBOs_[i]) {
            glDeleteFramebuffers(1, &pingPongFBOs_[i]);
            pingPongFBOs_[i] = 0;
        }
        if (pingPongTextures_[i]) {
            glDeleteTextures(1, &pingPongTextures_[i]);
            pingPongTextures_[i] = 0;
        }
    }
}

void PostProcessPipeline::InitFullscreenQuad() {
    float quadVertices[] = {
        // positions   // texcoords
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
}

}  // namespace se
