#pragma once

#include <glm.hpp>
#include <memory>
#include <cstdint>

namespace se {

class ComputeShader;

// Configuration for SSGI pass
struct SSGIConfig {
    bool  Enabled          = false;
    float ResolutionScale  = 0.5f;   // Half-res by default
    int   RayCount         = 8;      // Rays per pixel
    int   StepsPerRay      = 16;     // Raymarch steps per ray
    float MaxDistance      = 20.0f;  // Maximum ray distance in world units
    float Intensity        = 1.0f;   // Output multiplier
    float FalloffExponent  = 2.0f;   // Distance falloff
    int   BlurRadius       = 2;      // Bilateral blur radius
    float DepthThreshold   = 0.1f;   // Bilateral blur depth threshold
    float NormalThreshold  = 0.9f;   // Bilateral blur normal threshold (cos angle)
    int   DebugMode        = 0;      // 0=off, 1=SH coeffs, 2=raw radiance, 3=normals
};

// SSGI Pass using Horizon-Based Indirect Lighting with Spherical Harmonics
class SSGIPass {
public:
    SSGIPass();
    ~SSGIPass();

    SSGIPass(const SSGIPass&)            = delete;
    SSGIPass& operator=(const SSGIPass&) = delete;

    bool Init(int width, int height);
    void Shutdown();
    void Resize(int width, int height);

    // Execute the SSGI pass
    // Inputs: GBuffer textures (position, normal, albedo, emissive, depth)
    // Outputs: Final radiance texture accessible via GetRadianceTexture()
    void Execute(
        uint32_t positionTex,
        uint32_t normalTex,
        uint32_t albedoTex,
        uint32_t emissiveTex,
        uint32_t depthTex,
        const glm::mat4& projection,
        const glm::mat4& view,
        const glm::mat4& invProjection,
        const glm::mat4& invView,
        const glm::vec3& cameraPos
    );

    // Configuration
    void SetEnabled(bool enabled) { config_.Enabled = enabled; }
    bool IsEnabled() const { return config_.Enabled; }
    bool IsReady() const { return config_.Enabled && initialized_; }
    
    SSGIConfig& GetConfig() { return config_; }
    const SSGIConfig& GetConfig() const { return config_; }
    void SetConfig(const SSGIConfig& config) { config_ = config; }

    // Output texture
    uint32_t GetRadianceTexture() const { return finalRadianceTex_; }
    
    // Debug textures
    uint32_t GetSHTexture(int index) const { 
        return (index >= 0 && index < 4) ? shCoeffTex_[index] : 0; 
    }
    
    // Debug info
    bool IsInitialized() const { return initialized_; }
    int GetWorkWidth() const { return workWidth_; }
    int GetWorkHeight() const { return workHeight_; }

private:
    void CreateTextures();
    void DestroyTextures();
    void RaymarchAndEncode();
    void BilateralResolve();

    bool initialized_ = false;
    
    // Screen dimensions
    int screenWidth_  = 0;
    int screenHeight_ = 0;
    int workWidth_    = 0;  // width * resolution_scale
    int workHeight_   = 0;  // height * resolution_scale

    // Configuration
    SSGIConfig config_;

    // SH coefficient textures (L1 = 4 coefficients)
    // Each stores RGB values for one SH coefficient
    uint32_t shCoeffTex_[4] = {0, 0, 0, 0};
    
    // Final resolved radiance at full resolution
    uint32_t finalRadianceTex_ = 0;
    
    // Intermediate textures
    uint32_t rawRadianceTex_  = 0;  // Pre-SH radiance accumulation at work res
    uint32_t tempBlurTex_     = 0;  // Temp for separable blur
    
    // Compute shaders
    std::shared_ptr<ComputeShader> raymarchShader_;
    std::shared_ptr<ComputeShader> resolveShader_;
    
    // Cached input textures for resolve pass
    uint32_t lastPositionTex_ = 0;
    uint32_t lastNormalTex_   = 0;
    uint32_t lastAlbedoTex_   = 0;
    uint32_t lastEmissiveTex_ = 0;
    uint32_t lastDepthTex_    = 0;
    
    // Cached matrices
    glm::mat4 projection_;
    glm::mat4 view_;
    glm::mat4 invProjection_;
    glm::mat4 invView_;
    glm::vec3 cameraPos_;
    
    // Light data for sun contribution in GI
    glm::vec3 lightDirection_ = glm::vec3(0.3f, 1.0f, 0.2f);
    glm::vec3 lightColor_     = glm::vec3(1.0f, 0.95f, 0.9f);
    float lightIntensity_     = 1.0f;

public:
    // Set light data for sun contribution in GI calculation
    void SetLightData(const glm::vec3& direction, const glm::vec3& color, float intensity) {
        lightDirection_ = direction;
        lightColor_ = color;
        lightIntensity_ = intensity;
    }
};

} // namespace se
