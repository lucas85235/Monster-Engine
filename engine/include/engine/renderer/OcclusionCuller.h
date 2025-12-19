#pragma once

#include <glm.hpp>
#include <memory>
#include <unordered_map>
#include <vector>

#include "engine/renderer/Frustum.h"
#include "engine/renderer/OcclusionQuery.h"
#include "engine/renderer/VertexArray.h"

namespace se {

class Shader;

/**
 * OcclusionCuller - GPU-based occlusion culling using hardware queries.
 *
 * OcclusionCuller - GPU-based occlusion culling using hardware queries.
 *
 * Current implementation uses synchronous (blocking) queries:
 * 1. Render occluders (large objects).
 * 2. Render bounding boxes of occludees with queries.
 * 3. Wait for results (blocking).
 * 4. Render visible occludees.
 *
 * Note: This prevents popping but introduces a CPU-GPU sync point (stall).
 */
class OcclusionCuller {
   public:
    OcclusionCuller();
    ~OcclusionCuller();

    OcclusionCuller(const OcclusionCuller&)            = delete;
    OcclusionCuller& operator=(const OcclusionCuller&) = delete;

    void Init();
    void Shutdown();

    // Set the view-projection matrix for frustum extraction
    void SetViewProjection(const Matrix4& viewProj);

    // Get the frustum for external frustum culling
    const Frustum& GetFrustum() const {
        return frustum_;
    }

    // Test if a bounding sphere is visible (frustum only, fast)
    bool IsSphereVisible(const Vector3& center, float radius) const;

    // Test if an AABB is visible (frustum only, fast)
    bool IsAABBVisible(const Vector3& min, const Vector3& max) const;

    // Check if object was visible in previous frame (for temporal coherence)
    bool WasVisibleLastFrame(uint32_t objectId) const;

    // Begin occlusion query for an object's bounding box
    void BeginQuery(uint32_t objectId);

    // Render bounding box for the current query
    void RenderBoundingBox(const Vector3& center, const Vector3& halfExtents);

    // End current query
    void EndQuery();

    // Collect query results at end of frame
    void CollectResults();

    // Reset for new frame
    void BeginFrame();

    // Get current view-projection matrix
    const Matrix4& GetViewProjection() const {
        return viewProj_;
    }

    // Statistics
    uint32_t GetQueriesIssued() const {
        return queriesIssued_;
    }
    uint32_t GetOccludedCount() const {
        return occludedCount_;
    }

    void ResetStats() {
        queriesIssued_ = 0;
        occludedCount_ = 0;
    }

    // Enable/disable occlusion culling
    void SetEnabled(bool enabled) {
        enabled_ = enabled;
    }
    bool IsEnabled() const {
        return enabled_;
    }

   private:
    Frustum                             frustum_;
    Matrix4                             viewProj_{1.0f};
    std::unique_ptr<OcclusionQueryPool> queryPool_;

    // Bounding box for occlusion tests
    std::shared_ptr<VertexArray> boundingBoxVA_;
    std::shared_ptr<Shader>      occlusionShader_;

    // Previous frame visibility results
    std::unordered_map<uint32_t, bool> previousFrameVisibility_;

    // Current frame active queries
    std::unordered_map<uint32_t, std::shared_ptr<IOcclusionQuery>> activeQueries_;
    uint32_t                                                       currentQueryObjectId_ = 0;

    // Statistics
    uint32_t queriesIssued_ = 0;
    uint32_t occludedCount_ = 0;

    bool enabled_     = true;
    bool initialized_ = false;
};

}  // namespace se
