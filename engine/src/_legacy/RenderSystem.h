
#pragma once

#include <array>
#include <glm.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

#include "engine/Camera.h"
#include "engine/renderer/InstancedMesh.h"

namespace se {

// Forward declarations
class Scene;
class VertexArray;
class Material;
class Shader;

// Key for grouping instances by mesh+material
struct InstanceBatchKey {
    VertexArray* va;
    Material*    mat;

    bool operator==(const InstanceBatchKey& other) const {
        return va == other.va && mat == other.mat;
    }
};

// Hash function for InstanceBatchKey
struct InstanceBatchKeyHash {
    size_t operator()(const InstanceBatchKey& key) const {
        return std::hash<void*>()(key.va) ^ (std::hash<void*>()(key.mat) << 1);
    }
};

class RenderSystem {
   public:
    static void Init();
    static void Shutdown();

    // Render all entities with MeshRenderComponent in the scene
    static void Render(Scene& scene, const Camera& camera, float aspectRatio);

    // Get instancing statistics
    static uint32_t GetLastBatchCount() {
        return lastBatchCount_;
    }
    static uint32_t GetLastInstancedObjects() {
        return lastInstancedObjects_;
    }

   private:
    RenderSystem() = delete;
    static bool initialized_;

    // Instancing cache - reused each frame to avoid allocations
    using InstanceBatchMap =
        std::unordered_map<InstanceBatchKey, std::vector<InstanceData>, InstanceBatchKeyHash>;
    static InstanceBatchMap instanceBatches_;

    // Cache of InstancedMesh objects per batch key
    using InstancedMeshCache =
        std::unordered_map<InstanceBatchKey, std::shared_ptr<InstancedMesh>, InstanceBatchKeyHash>;
    static InstancedMeshCache instancedMeshCache_;

    // Stats
    static uint32_t lastBatchCount_;
    static uint32_t lastInstancedObjects_;

    // Instanced rendering material (uses instanced.vert/frag shader)
    static std::shared_ptr<Material> instancedMaterial_;
    static void                      EnsureInstancedMaterial();

    // Model rendering material (uses model.vert/frag shader)
    static std::shared_ptr<Material> modelMaterial_;
    static void                      EnsureModelMaterial();

    // Skinned model rendering material (uses skinned_model.vert/frag shader)
    static std::shared_ptr<Material> skinnedMaterial_;
    static void                      EnsureSkinnedMaterial();
    
    // Pre-computed bone matrix uniform locations for fast setting
    static constexpr size_t MAX_BONES = 256;
    static std::array<int, MAX_BONES> boneUniformLocations_;
    static bool                       boneLocationsInitialized_;
    static uint32_t                   boneUniformShaderID_;  // Track which shader the locations are for
    static void                       InitBoneUniformLocations(Shader* shader);
    
    // Track instance counts from last frame for reserve() optimization
    static std::unordered_map<InstanceBatchKey, size_t, InstanceBatchKeyHash> lastFrameInstanceCounts_;
    
    // Cache shared_ptrs and emissive properties per batch to avoid linear search
    struct BatchResources {
        std::shared_ptr<VertexArray> va;
        std::shared_ptr<Material>    material;
        glm::vec3                    emissiveColor{0.0f};
        float                        emissiveFactor = 0.0f;
    };
    using BatchResourcesCache = std::unordered_map<InstanceBatchKey, BatchResources, InstanceBatchKeyHash>;
    static BatchResourcesCache batchResources_;
};

}  // namespace se
