#pragma once

#include <cstdint>
#include <memory>
#include <glm.hpp>

namespace se {

class Shader;

class GBufferPass {
public:
    GBufferPass();
    ~GBufferPass();
    
    void Init(int width, int height);
    void Shutdown();
    void Resize(int width, int height);
    
    void Bind();
    void Unbind();
    void Clear();
    
    // Get textures for reading
    uint32_t GetPositionTexture() const { return positionTex_; }
    uint32_t GetNormalTexture() const { return normalTex_; }
    uint32_t GetAlbedoTexture() const { return albedoTex_; }
    uint32_t GetDepthTexture() const { return depthTex_; }
    uint32_t GetEmissiveTexture() const { return emissiveTex_; }
    
    int GetWidth() const { return width_; }
    int GetHeight() const { return height_; }
    
    bool IsInitialized() const { return initialized_; }

private:
    void CreateResources();
    void DestroyResources();
    
    uint32_t fbo_ = 0;
    uint32_t positionTex_ = 0;    // RGB: world position
    uint32_t normalTex_ = 0;      // RGB: world normal
    uint32_t albedoTex_ = 0;      // RGBA: albedo + alpha
    uint32_t emissiveTex_ = 0;    // RGB: emissive color
    uint32_t depthTex_ = 0;       // Depth buffer
    
    int width_ = 0;
    int height_ = 0;
    bool initialized_ = false;
};

}  // namespace se
