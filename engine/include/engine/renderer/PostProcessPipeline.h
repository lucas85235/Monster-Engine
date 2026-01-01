#pragma once

#include <memory>
#include <vector>
#include <glad/glad.h>

#include "engine/renderer/PostProcessPass.h"
#include "engine/Shader.h"

namespace se {

class PostProcessPipeline {
public:
    PostProcessPipeline() = default;
    ~PostProcessPipeline();

    void Init(uint32_t width, uint32_t height);
    void Shutdown();
    void Resize(uint32_t width, uint32_t height);

    void Execute(GLuint sceneTexture, GLuint sceneDepth, GLuint targetFBO = 0, 
                 uint32_t targetWidth = 0, uint32_t targetHeight = 0);

    GLuint GetOutputTexture() const { return pingPongTextures_[currentOutput_]; }

    template <typename T, typename... Args>
    T* AddPass(Args&&... args) {
        auto pass = std::make_unique<T>(std::forward<Args>(args)...);
        T* ptr = pass.get();
        if (initialized_) {
            pass->Init(width_, height_);
        }
        passes_.push_back(std::move(pass));
        return ptr;
    }

    template <typename T>
    T* GetPass() {
        for (auto& pass : passes_) {
            if (T* typed = dynamic_cast<T*>(pass.get())) {
                return typed;
            }
        }
        return nullptr;
    }

    void RenderUI();

    bool IsInitialized() const { return initialized_; }
    size_t GetPassCount() const { return passes_.size(); }

private:
    void CreateFramebuffers();
    void DestroyFramebuffers();
    void InitFullscreenQuad();

    std::vector<std::unique_ptr<PostProcessPass>> passes_;
    
    GLuint pingPongFBOs_[2] = {0, 0};
    GLuint pingPongTextures_[2] = {0, 0};
    int currentOutput_ = 0;

    GLuint quadVAO_ = 0;
    GLuint quadVBO_ = 0;

    uint32_t width_ = 0;
    uint32_t height_ = 0;
    bool initialized_ = false;
};

}  // namespace se
