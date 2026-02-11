#pragma once

#include <memory>
#include <vector>
#include <cstdint>
#include <glm.hpp>

#include "engine/renderer/gi/SparseBrickCache.h"
#include "engine/renderer/gi/RCProfiler.h"
#include "engine/renderer/gi/RCDebugRenderer.h"

namespace se {

class ComputeShader;
class SceneVoxelizer;
class GBufferPass;

namespace gi {
class IWorldRayQuery;
class VoxelRayQueryBackend;
}

struct SparseRCConfig {
    int NumCascades = 4;
    int BrickSize = 8;
    int BaseBrickCount = 32;
    float BaseVoxelSize = 0.5f;
    int SHSamplesPerProbe = 64;
    int UpdateBudget = 128;
    float CascadeScale = 2.0f;
    float GIIntensity = 1.0f;
    float ActivationRadius = 50.0f;
    bool TemporalAccumulation = false;
    bool Enabled = true;
    
    gi::RCDebugMode DebugMode = gi::RCDebugMode::Off;
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
    SparseRCConfig& GetConfig() { return config_; }
    
    void SetGBufferPass(GBufferPass* gbuffer) { gbuffer_ = gbuffer; }
    
    void Execute(const glm::mat4& projection, const glm::mat4& view, 
                 const glm::vec3& cameraPos, uint32_t frameNumber);
    
    uint32_t GetRadianceTexture() const { return finalRadianceTex_; }
    
    bool IsEnabled() const { return config_.Enabled && initialized_; }
    void SetEnabled(bool enabled) { config_.Enabled = enabled; }
    
    gi::SparseBrickCache* GetBrickCache() { return brickCache_.get(); }
    gi::RCProfiler* GetProfiler() { return profiler_.get(); }
    gi::RCDebugRenderer* GetDebugRenderer() { return debugRenderer_.get(); }
    
    void RenderDebug(const glm::mat4& viewProj);
    std::string GetProfileSummary() const;
    
private:
    void CreateResources();
    void DestroyResources();
    void LoadShaders();
    
    void UpdateBrickActivation(const glm::vec3& cameraPos);
    void PopulateCascades(const glm::vec3& cameraPos);
    void MergeCascades();
    void ResolveToScreen();
    
    float GetCascadeVoxelSize(int cascadeIndex) const;
    void GetCascadeInterval(int cascadeIndex, float& start, float& end) const;
    
    SparseRCConfig config_;
    bool initialized_ = false;
    int screenWidth_ = 0;
    int screenHeight_ = 0;
    uint32_t currentFrame_ = 0;
    
    std::shared_ptr<SceneVoxelizer> voxelizer_;
    GBufferPass* gbuffer_ = nullptr;
    
    std::unique_ptr<gi::SparseBrickCache> brickCache_;
    std::unique_ptr<gi::VoxelRayQueryBackend> rayQuery_;
    std::unique_ptr<gi::RCProfiler> profiler_;
    std::unique_ptr<gi::RCDebugRenderer> debugRenderer_;
    
    std::vector<uint32_t> cascadeTextures_;
    uint32_t finalRadianceTex_ = 0;
    
    std::shared_ptr<ComputeShader> populateShader_;
    std::shared_ptr<ComputeShader> mergeShader_;
    std::shared_ptr<ComputeShader> resolveShader_;
    
    glm::mat4 invProjection_;
    glm::mat4 invView_;
    glm::vec3 cameraPos_;
    
    // Cached grid params used during populate - must use same values in resolve
    glm::vec3 cachedGridCenter_ = glm::vec3(0.0f);
    float cachedGridSize_ = 50.0f;
};


}  // namespace se
