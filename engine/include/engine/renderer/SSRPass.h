#pragma once
/**
 * SSRPass.h - Screen Space Reflections post-process
 * 
 * Ray marches in screen space to find reflections for glossy surfaces.
 * Should be combined with IBL for surfaces that miss or are outside screen.
 */

#include <memory>
#include <cstdint>
#include <glm.hpp>

namespace se {

class Shader;

struct SSRSettings {
    float maxDistance = 50.0f;
    float thickness = 0.5f;
    int maxSteps = 64;
    float roughnessThreshold = 0.5f;
    bool enabled = true;
};

class SSRPass {
public:
    SSRPass();
    ~SSRPass();
    
    bool Initialize(int width, int height);
    void Shutdown();
    void Resize(int width, int height);
    
    void Apply(
        uint32_t colorTexture,
        uint32_t depthTexture,
        uint32_t normalTexture,
        uint32_t roughnessTexture,
        const glm::mat4& viewMatrix,
        const glm::mat4& projectionMatrix
    );
    
    uint32_t GetResultTexture() const { return resultTexture_; }
    
    SSRSettings& GetSettings() { return settings_; }
    const SSRSettings& GetSettings() const { return settings_; }
    void SetSettings(const SSRSettings& settings) { settings_ = settings; }
    
    bool IsEnabled() const { return settings_.enabled; }
    void SetEnabled(bool enabled) { settings_.enabled = enabled; }

private:
    void CreateResources(int width, int height);
    void DestroyResources();
    void RenderFullscreenQuad();
    
    std::shared_ptr<Shader> shader_;
    
    uint32_t fbo_ = 0;
    uint32_t resultTexture_ = 0;
    
    SSRSettings settings_;
    int width_ = 0;
    int height_ = 0;
    
    uint32_t quadVAO_ = 0;
    uint32_t quadVBO_ = 0;
    bool initialized_ = false;
};

} // namespace se
