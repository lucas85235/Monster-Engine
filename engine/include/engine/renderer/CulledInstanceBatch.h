#pragma once

#include <memory>
#include <vector>

#include <glm.hpp>

#include "engine/renderer/Frustum.h"
#include "engine/renderer/IInstanceBuffer.h"
#include "engine/renderer/InstancedMesh.h"
#include "engine/renderer/Material.h"
#include "engine/renderer/VertexArray.h"

namespace se {

/**
 * CulledInstanceBatch - Manages a large number of instances with frustum culling.
 * 
 * This class maintains a list of all instances and performs frustum culling
 * each frame to only render visible instances.
 */
class CulledInstanceBatch {
   public:
    struct Instance {
        Vector3 position{0.0f};
        Vector3 scale{1.0f};
        Vector4 color{1.0f};
        float   boundingRadius = 1.0f;  // For frustum culling
        bool    visible        = true;  // CPU-side visibility flag
    };

    CulledInstanceBatch(const std::shared_ptr<VertexArray>& baseVA, uint32_t maxInstances);
    ~CulledInstanceBatch() = default;

    CulledInstanceBatch(const CulledInstanceBatch&)            = delete;
    CulledInstanceBatch& operator=(const CulledInstanceBatch&) = delete;

    void SetInstances(const std::vector<Instance>& instances);
    void AddInstance(const Instance& instance);
    void ClearInstances();

    // Perform frustum culling and update GPU buffer
    void CullAndUpdate(const Frustum& frustum);

    // Draw visible instances (call after CullAndUpdate)
    void Draw(const std::shared_ptr<Material>& material);
    void DrawWithoutMaterial();

    // Statistics
    uint32_t GetTotalInstances() const { return static_cast<uint32_t>(allInstances_.size()); }
    uint32_t GetVisibleInstances() const { return visibleCount_; }
    uint32_t GetCulledInstances() const { return GetTotalInstances() - visibleCount_; }
    float    GetCullRatio() const {
        return GetTotalInstances() > 0 ? static_cast<float>(GetCulledInstances()) / GetTotalInstances() : 0.0f;
    }

   private:
    void RebuildVisibleBuffer();

    std::shared_ptr<VertexArray>    instancedVA_;
    std::shared_ptr<IInstanceBuffer> instanceBuffer_;
    std::vector<Instance>           allInstances_;
    std::vector<InstanceData>       visibleData_;  // Temp buffer for visible instances
    uint32_t maxInstances_  = 0;
    uint32_t visibleCount_  = 0;
    bool     needsRebuild_  = true;
};

}  // namespace se
