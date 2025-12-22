
#pragma once

#include <glm.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

#include "engine/renderer/Camera.h"
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
    float        boundingRadius;  // Helper for culling single instances

    bool operator==(const InstanceBatchKey& other) const {
        return va == other.va && mat == other.mat;  // Radius should match if va matches
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
    using InstanceBatchMap = std::unordered_map<InstanceBatchKey, std::vector<InstanceData>, InstanceBatchKeyHash>;
    static InstanceBatchMap instanceBatches_;

    // Cache of InstancedMesh objects per batch key
    using InstancedMeshCache = std::unordered_map<InstanceBatchKey, std::shared_ptr<InstancedMesh>, InstanceBatchKeyHash>;
    static InstancedMeshCache instancedMeshCache_;

    // Stats
    static uint32_t lastBatchCount_;
    static uint32_t lastInstancedObjects_;

    // Instanced rendering material (uses instanced.vert/frag shader)
    static std::shared_ptr<Material> instancedMaterial_;
    static void                      EnsureInstancedMaterial();
};

}  // namespace se
