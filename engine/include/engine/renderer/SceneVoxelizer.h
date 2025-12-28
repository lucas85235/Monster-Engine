#pragma once

#include <memory>
#include <cstdint>
#include <glm.hpp>

namespace se {

class Scene;
class ComputeShader;

struct VoxelGridConfig {
    int Resolution = 128;          // Grid resolution (128³)
    float WorldSize = 50.0f;       // World units covered by grid
    glm::vec3 Center{0.0f};        // Grid center in world space
    bool UpdateEveryFrame = true;  // Dynamic scene support
};

class SceneVoxelizer {
public:
    SceneVoxelizer();
    ~SceneVoxelizer();
    
    void Init(const VoxelGridConfig& config);
    void Shutdown();
    
    void Voxelize(Scene& scene, const glm::mat4& viewMatrix, const glm::mat4& projMatrix);
    void Clear();
    
    uint32_t GetVoxelTexture() const { return voxelTexture_; }
    uint32_t GetVoxelEmissiveTexture() const { return voxelEmissiveTexture_; }
    
    const VoxelGridConfig& GetConfig() const { return config_; }
    void SetCenter(const glm::vec3& center) { config_.Center = center; }
    
    bool IsInitialized() const { return initialized_; }
    
    // Convert world position to voxel coordinates
    glm::ivec3 WorldToVoxel(const glm::vec3& worldPos) const;
    glm::vec3 VoxelToWorld(const glm::ivec3& voxelPos) const;
    
private:
    void CreateTextures();
    void DestroyTextures();
    void LoadShaders();
    
    VoxelGridConfig config_;
    bool initialized_ = false;
    
    // 3D textures for voxel data
    uint32_t voxelTexture_ = 0;         // RGBA8: albedo + opacity
    uint32_t voxelEmissiveTexture_ = 0; // RGBA16F: emissive color + intensity
    
    // Voxelization shaders
    std::shared_ptr<ComputeShader> clearShader_;
    std::shared_ptr<ComputeShader> voxelizeShader_;
    
    // Framebuffer for voxelization (geometry shader approach)
    uint32_t voxelFBO_ = 0;
};

}  // namespace se
