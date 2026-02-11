#include "engine/renderer/gi/VoxelRayQueryBackend.h"
#include "engine/renderer/SceneVoxelizer.h"
#include "engine/Log.h"

#include <glad/glad.h>
#include <cmath>
#include <algorithm>

namespace se {
namespace gi {

VoxelRayQueryBackend::VoxelRayQueryBackend() {
    SE_LOG_INFO("VoxelRayQueryBackend created");
}

VoxelRayQueryBackend::~VoxelRayQueryBackend() {
    SE_LOG_INFO("VoxelRayQueryBackend destroyed");
}

void VoxelRayQueryBackend::SetVoxelizer(std::shared_ptr<SceneVoxelizer> voxelizer) {
    voxelizer_ = voxelizer;
    dataCached_ = false;
    
    if (voxelizer_ && voxelizer_->IsInitialized()) {
        const auto& config = voxelizer_->GetConfig();
        gridCenter_ = config.Center;
        gridWorldSize_ = config.WorldSize;
        gridResolution_ = config.Resolution;
        voxelSize_ = gridWorldSize_ / static_cast<float>(gridResolution_);
        
        SE_LOG_INFO("VoxelRayQueryBackend configured: center=({:.2f},{:.2f},{:.2f}), size={:.2f}, res={}", 
                    gridCenter_.x, gridCenter_.y, gridCenter_.z, gridWorldSize_, gridResolution_);
    }
}

void VoxelRayQueryBackend::SetDirectionalLight(const glm::vec3& direction, 
                                                const glm::vec3& color, 
                                                float intensity) {
    lightDirection_ = glm::normalize(direction);
    lightColor_ = color;
    lightIntensity_ = intensity;
}

bool VoxelRayQueryBackend::IsReady() const {
    return voxelizer_ != nullptr && voxelizer_->IsInitialized() && dataCached_;
}

void VoxelRayQueryBackend::Update() {
    if (!voxelizer_ || !voxelizer_->IsInitialized()) {
        dataCached_ = false;
        return;
    }
    
    const auto& config = voxelizer_->GetConfig();
    gridCenter_ = config.Center;
    gridWorldSize_ = config.WorldSize;
    gridResolution_ = config.Resolution;
    voxelSize_ = gridWorldSize_ / static_cast<float>(gridResolution_);
    
    const int totalVoxels = gridResolution_ * gridResolution_ * gridResolution_;
    
    cpuVoxelData_.resize(totalVoxels * 4);
    cpuEmissiveData_.resize(totalVoxels * 4);
    
    glBindTexture(GL_TEXTURE_3D, voxelizer_->GetVoxelTexture());
    glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_UNSIGNED_BYTE, cpuVoxelData_.data());
    
    glBindTexture(GL_TEXTURE_3D, voxelizer_->GetVoxelEmissiveTexture());
    glGetTexImage(GL_TEXTURE_3D, 0, GL_RGBA, GL_FLOAT, cpuEmissiveData_.data());
    
    glBindTexture(GL_TEXTURE_3D, 0);
    
    dataCached_ = true;
}

glm::ivec3 VoxelRayQueryBackend::WorldToVoxel(const glm::vec3& worldPos) const {
    glm::vec3 localPos = worldPos - gridCenter_;
    glm::vec3 normalizedPos = (localPos / gridWorldSize_) + 0.5f;
    return glm::ivec3(normalizedPos * static_cast<float>(gridResolution_));
}

glm::vec3 VoxelRayQueryBackend::VoxelToWorld(const glm::ivec3& voxelPos) const {
    glm::vec3 normalizedPos = glm::vec3(voxelPos) / static_cast<float>(gridResolution_);
    glm::vec3 localPos = (normalizedPos - 0.5f) * gridWorldSize_;
    return localPos + gridCenter_;
}

bool VoxelRayQueryBackend::IsInsideGrid(const glm::ivec3& voxelPos) const {
    return voxelPos.x >= 0 && voxelPos.x < gridResolution_ &&
           voxelPos.y >= 0 && voxelPos.y < gridResolution_ &&
           voxelPos.z >= 0 && voxelPos.z < gridResolution_;
}

bool VoxelRayQueryBackend::SampleVoxelOccupancy(const glm::ivec3& voxelPos) const {
    if (!IsInsideGrid(voxelPos)) return false;
    
    const int idx = (voxelPos.z * gridResolution_ * gridResolution_ + 
                     voxelPos.y * gridResolution_ + 
                     voxelPos.x) * 4;
    
    return cpuVoxelData_[idx + 3] > 25;
}

glm::vec4 VoxelRayQueryBackend::SampleVoxelAlbedo(const glm::ivec3& voxelPos) const {
    if (!IsInsideGrid(voxelPos)) return glm::vec4(0.0f);
    
    const int idx = (voxelPos.z * gridResolution_ * gridResolution_ + 
                     voxelPos.y * gridResolution_ + 
                     voxelPos.x) * 4;
    
    return glm::vec4(
        cpuVoxelData_[idx + 0] / 255.0f,
        cpuVoxelData_[idx + 1] / 255.0f,
        cpuVoxelData_[idx + 2] / 255.0f,
        cpuVoxelData_[idx + 3] / 255.0f
    );
}

glm::vec4 VoxelRayQueryBackend::SampleVoxelEmissive(const glm::ivec3& voxelPos) const {
    if (!IsInsideGrid(voxelPos)) return glm::vec4(0.0f);
    
    const int idx = (voxelPos.z * gridResolution_ * gridResolution_ + 
                     voxelPos.y * gridResolution_ + 
                     voxelPos.x) * 4;
    
    return glm::vec4(
        cpuEmissiveData_[idx + 0],
        cpuEmissiveData_[idx + 1],
        cpuEmissiveData_[idx + 2],
        cpuEmissiveData_[idx + 3]
    );
}

RayHit VoxelRayQueryBackend::TraceSegment(const glm::vec3& origin, 
                                           const glm::vec3& direction,
                                           float tMin, 
                                           float tMax) {
    RayHit result;
    result.hit = false;
    
    if (!dataCached_) {
        return result;
    }
    
    glm::vec3 dir = glm::normalize(direction);
    glm::vec3 rayStart = origin + dir * tMin;
    
    glm::ivec3 currentVoxel = WorldToVoxel(rayStart);
    
    glm::ivec3 step;
    step.x = (dir.x >= 0) ? 1 : -1;
    step.y = (dir.y >= 0) ? 1 : -1;
    step.z = (dir.z >= 0) ? 1 : -1;
    
    glm::vec3 tDelta;
    tDelta.x = (std::abs(dir.x) > 1e-6f) ? voxelSize_ / std::abs(dir.x) : 1e30f;
    tDelta.y = (std::abs(dir.y) > 1e-6f) ? voxelSize_ / std::abs(dir.y) : 1e30f;
    tDelta.z = (std::abs(dir.z) > 1e-6f) ? voxelSize_ / std::abs(dir.z) : 1e30f;
    
    glm::vec3 voxelMin = VoxelToWorld(currentVoxel) - glm::vec3(voxelSize_ * 0.5f);
    glm::vec3 voxelMax = voxelMin + glm::vec3(voxelSize_);
    
    glm::vec3 tMax_axis;
    tMax_axis.x = (step.x > 0) ? (voxelMax.x - rayStart.x) / dir.x : (voxelMin.x - rayStart.x) / dir.x;
    tMax_axis.y = (step.y > 0) ? (voxelMax.y - rayStart.y) / dir.y : (voxelMin.y - rayStart.y) / dir.y;
    tMax_axis.z = (step.z > 0) ? (voxelMax.z - rayStart.z) / dir.z : (voxelMin.z - rayStart.z) / dir.z;
    
    if (std::abs(dir.x) < 1e-6f) tMax_axis.x = 1e30f;
    if (std::abs(dir.y) < 1e-6f) tMax_axis.y = 1e30f;
    if (std::abs(dir.z) < 1e-6f) tMax_axis.z = 1e30f;
    
    float t = 0.0f;
    const float maxT = tMax - tMin;
    const int maxSteps = gridResolution_ * 3;
    
    for (int i = 0; i < maxSteps && t < maxT; ++i) {
        if (IsInsideGrid(currentVoxel) && SampleVoxelOccupancy(currentVoxel)) {
            result.hit = true;
            result.distance = tMin + t;
            result.position = origin + dir * result.distance;
            
            glm::vec4 albedo = SampleVoxelAlbedo(currentVoxel);
            result.albedo = glm::vec3(albedo);
            
            glm::vec4 emissive = SampleVoxelEmissive(currentVoxel);
            result.emissive = glm::vec3(emissive);
            
            glm::vec3 voxelCenter = VoxelToWorld(currentVoxel);
            glm::vec3 hitOffset = result.position - voxelCenter;
            glm::vec3 absOffset = glm::abs(hitOffset);
            
            if (absOffset.x >= absOffset.y && absOffset.x >= absOffset.z) {
                result.normal = glm::vec3((hitOffset.x > 0) ? 1.0f : -1.0f, 0.0f, 0.0f);
            } else if (absOffset.y >= absOffset.x && absOffset.y >= absOffset.z) {
                result.normal = glm::vec3(0.0f, (hitOffset.y > 0) ? 1.0f : -1.0f, 0.0f);
            } else {
                result.normal = glm::vec3(0.0f, 0.0f, (hitOffset.z > 0) ? 1.0f : -1.0f);
            }
            
            return result;
        }
        
        if (tMax_axis.x < tMax_axis.y && tMax_axis.x < tMax_axis.z) {
            t = tMax_axis.x;
            tMax_axis.x += tDelta.x;
            currentVoxel.x += step.x;
        } else if (tMax_axis.y < tMax_axis.z) {
            t = tMax_axis.y;
            tMax_axis.y += tDelta.y;
            currentVoxel.y += step.y;
        } else {
            t = tMax_axis.z;
            tMax_axis.z += tDelta.z;
            currentVoxel.z += step.z;
        }
        
        if (!IsInsideGrid(currentVoxel)) {
            break;
        }
    }
    
    return result;
}

DirectLightSample VoxelRayQueryBackend::SampleDirectLighting(const glm::vec3& position,
                                                              const glm::vec3& normal) {
    DirectLightSample sample;
    
    float nDotL = glm::max(0.0f, glm::dot(normal, -lightDirection_));
    
    if (nDotL <= 0.0f) {
        sample.radiance = glm::vec3(0.0f);
        sample.visibility = 0.0f;
        return sample;
    }
    
    glm::vec3 shadowRayOrigin = position + normal * voxelSize_ * 0.5f;
    RayHit shadowHit = TraceSegment(shadowRayOrigin, -lightDirection_, 0.0f, gridWorldSize_);
    
    sample.visibility = shadowHit.hit ? 0.0f : 1.0f;
    sample.radiance = lightColor_ * lightIntensity_ * nDotL * sample.visibility;
    
    return sample;
}

}  // namespace gi
}  // namespace se
