#pragma once

#include <memory>
#include <vector>
#include <glm.hpp>

namespace se {

class ComputeShader;

struct RadianceCascadeConfig {
    int NumCascades = 4;
    int BaseProbeCount = 256;    // Probes in cascade 0 (per axis) - high for quality
    int BaseRayCount = 16;       // Rays per probe in cascade 0 - more for better angular coverage
    float IntervalLength = 64.0f; // Base ray length in pixels - long for better light propagation
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
    
    // Input: scene emissive, depth, and world position textures + voxel grid
    // Output: indirect lighting texture
    void Execute(uint32_t sceneColorTex, uint32_t sceneDepthTex, uint32_t scenePositionTex,
                 const glm::mat4& projection, const glm::mat4& view,
                 const glm::vec3& cameraPos,
                 uint32_t voxelAlbedoTex = 0, uint32_t voxelEmissiveTex = 0,
                 const glm::vec3& voxelGridCenter = glm::vec3(0.0f), 
                 float voxelGridSize = 50.0f, int voxelResolution = 128);
    
    // Get the resulting GI texture to composite into final image
    uint32_t GetRadianceTexture() const { return finalRadianceTex_; }
    
    bool IsEnabled() const { return config_.Enabled && initialized_; }
    void SetEnabled(bool enabled) { config_.Enabled = enabled; }

private:
    void CreateCascadeTextures();
    void DestroyCascadeTextures();
    void LoadShaders();
    
    void RaymarchCascade(int cascadeIndex, uint32_t sceneColorTex, uint32_t sceneDepthTex,
                         uint32_t scenePositionTex, const glm::vec3& cameraPos,
                         uint32_t voxelAlbedoTex, uint32_t voxelEmissiveTex,
                         const glm::vec3& voxelGridCenter, float voxelGridSize, int voxelResolution);
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
    
    // Temporal history for off-screen persistence
    uint32_t historyRadianceTex_ = 0;
    glm::mat4 prevViewProj_;
    bool hasPreviousFrame_ = false;
    
    // Compute shaders
    std::shared_ptr<ComputeShader> raymarchShader_;
    std::shared_ptr<ComputeShader> mergeShader_;
    std::shared_ptr<ComputeShader> resolveShader_;
    
    // Cached matrices
    glm::mat4 invProjection_;
    glm::mat4 invView_;
    
    // Cached textures for debug and temporal
    uint32_t lastEmissiveTex_ = 0;
    uint32_t lastPositionTex_ = 0;
};

}  // namespace se
