#pragma once

#include <memory>
#include <vector>
#include <glm.hpp>

namespace se {

class ComputeShader;

struct RadianceCascadeConfig {
    int NumCascades = 4;
    int BaseProbeCount = 64;     // Probes in cascade 0 (per axis)
    int BaseRayCount = 4;        // Rays per probe in cascade 0
    float IntervalLength = 4.0f;  // Base ray length in pixels
    float RayBias = 0.01f;
    bool Enabled = true;
};

class RadianceCascadesPass {
public:
    RadianceCascadesPass();
    ~RadianceCascadesPass();
    
    void Init(int screenWidth, int screenHeight);
    void Shutdown();
    void Resize(int screenWidth, int screenHeight);
    
    void SetConfig(const RadianceCascadeConfig& config);
    const RadianceCascadeConfig& GetConfig() const { return config_; }
    
    // Input: scene color and depth textures
    // Output: indirect lighting texture
    void Execute(uint32_t sceneColorTex, uint32_t sceneDepthTex, 
                 const glm::mat4& projection, const glm::mat4& view);
    
    // Get the resulting GI texture to composite into final image
    uint32_t GetRadianceTexture() const { return finalRadianceTex_; }
    
    bool IsEnabled() const { return config_.Enabled && initialized_; }
    void SetEnabled(bool enabled) { config_.Enabled = enabled; }

private:
    void CreateCascadeTextures();
    void DestroyCascadeTextures();
    void LoadShaders();
    
    void RaymarchCascade(int cascadeIndex, uint32_t sceneColorTex, uint32_t sceneDepthTex);
    void MergeCascades();
    void ResolveRadiance();

    RadianceCascadeConfig config_;
    bool initialized_ = false;
    int screenWidth_ = 0;
    int screenHeight_ = 0;
    
    // Cascade textures (stores radiance per probe per ray)
    std::vector<uint32_t> cascadeTextures_;
    
    // Final radiance output
    uint32_t finalRadianceTex_ = 0;
    
    // Compute shaders
    std::shared_ptr<ComputeShader> raymarchShader_;
    std::shared_ptr<ComputeShader> mergeShader_;
    std::shared_ptr<ComputeShader> resolveShader_;
    
    // Cached matrices
    glm::mat4 invProjection_;
    glm::mat4 invView_;
};

}  // namespace se
