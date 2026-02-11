#pragma once
/**
 * TAAPass.h - Temporal Anti-Aliasing
 * 
 * Blends current frame with history using motion vectors.
 * Requires jittered projection matrix and motion vector pass.
 */

#include <memory>
#include <cstdint>
#include <glm.hpp>

namespace se {

class Shader;

struct TAASettings {
    float blendFactor = 0.9f;
    int jitterSequenceLength = 8;
    bool enabled = true;
};

class TAAPass {
public:
    TAAPass();
    ~TAAPass();
    
    bool Initialize(int width, int height);
    void Shutdown();
    void Resize(int width, int height);
    
    void Apply(
        uint32_t currentFrame,
        uint32_t depthTexture,
        uint32_t motionVectors
    );
    
    uint32_t GetResultTexture() const;
    
    glm::vec2 GetCurrentJitter() const { return currentJitter_; }
    void AdvanceJitter();
    
    TAASettings& GetSettings() { return settings_; }
    const TAASettings& GetSettings() const { return settings_; }
    void SetSettings(const TAASettings& settings) { settings_ = settings; }
    
    bool IsEnabled() const { return settings_.enabled; }
    void SetEnabled(bool enabled) { settings_.enabled = enabled; }

private:
    void CreateResources(int width, int height);
    void DestroyResources();
    void RenderFullscreenQuad();
    glm::vec2 GetHaltonJitter(int index) const;
    
    std::shared_ptr<Shader> shader_;
    
    uint32_t fbo_[2] = {0, 0};
    uint32_t historyTexture_[2] = {0, 0};
    int currentBuffer_ = 0;
    
    TAASettings settings_;
    glm::vec2 currentJitter_ = glm::vec2(0.0f);
    int jitterIndex_ = 0;
    int width_ = 0;
    int height_ = 0;
    
    uint32_t quadVAO_ = 0;
    uint32_t quadVBO_ = 0;
    bool initialized_ = false;
};

} // namespace se
