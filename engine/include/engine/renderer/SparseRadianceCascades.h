#pragma once

#include <memory>
#include <vector>
#include <cstdint>
#include <glm.hpp>

namespace se {

class ComputeShader;
class SceneVoxelizer;

struct SparseRCConfig {
    int NumCascades = 4;           // Number of cascade levels
    int BaseProbeCount = 32;       // Probes per axis in cascade 0
    int DirectionsPerProbe = 6;    // Directions sampled per probe (6 = cube faces)
    float BaseInterval = 2.0f;     // Base ray interval in world units
    float CascadeScale = 4.0f;     // Scale factor between cascades
    bool Enabled = true;
};

class SparseRadianceCascades {
public:
    SparseRadianceCascades();
    ~SparseRadianceCascades();
    
    void Init(int screenWidth, int screenHeight, std::shared_ptr<SceneVoxelizer> voxelizer);
    void Shutdown();
    void Resize(int screenWidth, int screenHeight);
    
    void SetConfig(const SparseRCConfig& config);
    const SparseRCConfig& GetConfig() const { return config_; }
    
    // Execute GI computation using voxelized scene
    void Execute(const glm::mat4& projection, const glm::mat4& view, const glm::vec3& cameraPos);
    
    // Get final radiance texture for compositing
    uint32_t GetRadianceTexture() const { return finalRadianceTex_; }
    
    bool IsEnabled() const { return config_.Enabled && initialized_; }
    void SetEnabled(bool enabled) { config_.Enabled = enabled; }
    
private:
    void CreateCascadeTextures();
    void DestroyCascadeTextures();
    void LoadShaders();
    
    void RaymarchCascade(int cascadeIndex, const glm::vec3& cameraPos);
    void MergeCascades();
    void ResolveToScreen(const glm::mat4& invView);
    
    // Get probe count for specific cascade
    int GetProbeCount(int cascadeIndex) const;
    // Get interval range for specific cascade [start, end]
    void GetCascadeInterval(int cascadeIndex, float& start, float& end) const;
    
    SparseRCConfig config_;
    bool initialized_ = false;
    int screenWidth_ = 0;
    int screenHeight_ = 0;
    
    std::shared_ptr<SceneVoxelizer> voxelizer_;
    
    // Per-cascade 3D textures storing radiance per probe per direction
    // Layout: [probeX][probeY][probeZ * numDirections + direction]
    std::vector<uint32_t> cascadeTextures_;
    
    // Final screen-space radiance output
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
