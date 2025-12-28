#pragma once

#include "engine/renderer/gi/IWorldRayQuery.h"
#include <memory>
#include <cstdint>
#include <glm.hpp>

namespace se {

class SceneVoxelizer;

namespace gi {

class VoxelRayQueryBackend : public IWorldRayQuery {
public:
    VoxelRayQueryBackend();
    ~VoxelRayQueryBackend() override;

    void SetVoxelizer(std::shared_ptr<SceneVoxelizer> voxelizer);
    
    RayHit TraceSegment(const glm::vec3& origin, 
                        const glm::vec3& direction,
                        float tMin, 
                        float tMax) override;
    
    DirectLightSample SampleDirectLighting(const glm::vec3& position,
                                           const glm::vec3& normal) override;
    
    bool IsReady() const override;
    void Update() override;
    
    void SetDirectionalLight(const glm::vec3& direction, 
                             const glm::vec3& color, 
                             float intensity);

private:
    glm::ivec3 WorldToVoxel(const glm::vec3& worldPos) const;
    glm::vec3 VoxelToWorld(const glm::ivec3& voxelPos) const;
    bool IsInsideGrid(const glm::ivec3& voxelPos) const;
    
    bool SampleVoxelOccupancy(const glm::ivec3& voxelPos) const;
    glm::vec4 SampleVoxelAlbedo(const glm::ivec3& voxelPos) const;
    glm::vec4 SampleVoxelEmissive(const glm::ivec3& voxelPos) const;
    
    std::shared_ptr<SceneVoxelizer> voxelizer_;
    
    glm::vec3 gridCenter_{0.0f};
    float gridWorldSize_ = 50.0f;
    int gridResolution_ = 128;
    float voxelSize_ = 0.0f;
    
    glm::vec3 lightDirection_{0.0f, -1.0f, 0.0f};
    glm::vec3 lightColor_{1.0f};
    float lightIntensity_ = 1.0f;
    
    std::vector<uint8_t> cpuVoxelData_;
    std::vector<float> cpuEmissiveData_;
    bool dataCached_ = false;
};

}  // namespace gi
}  // namespace se
