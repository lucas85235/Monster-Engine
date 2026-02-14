#pragma once

#include <cstdint>
#include <limits>
#include <vector>

#include "engine/renderer/MaterialHandle.h"
#include <utils/Entity.h>

namespace filament {
class Engine;
class Scene;
class VertexBuffer;
class IndexBuffer;
} // namespace filament

namespace se {

struct MeshData;

/**
 * Result of creating a Filament renderable from MeshData.
 *
 * The renderable entity is already added to the Filament scene.
 * Destroy via MeshSystem::DestroyRenderable().
 */
struct RenderableHandle {
    static constexpr uint32_t kInvalidIndex = std::numeric_limits<uint32_t>::max();

    uint32_t index      = kInvalidIndex;
    uint32_t generation = 0;

    bool IsValid() const { return index != kInvalidIndex; }
};

/**
 * Creates and manages Filament renderables from engine MeshData.
 *
 * Handles VertexBuffer/IndexBuffer creation, Renderable building,
 * and adding renderables to the Filament scene.
 *
 * Thread Safety: Not thread-safe. Must be called from the main thread.
 */
class MeshSystem {
public:
    MeshSystem() = default;
    ~MeshSystem();

    // Non-copyable
    MeshSystem(const MeshSystem&) = delete;
    MeshSystem& operator=(const MeshSystem&) = delete;

    /**
     * Initialize with Filament engine and scene.
     */
    void Init(filament::Engine* engine, filament::Scene* scene);

    /**
     * Shutdown and destroy all managed renderables.
     */
    void Shutdown();

    /**
     * Create a renderable entity from mesh data and a material.
     * The renderable is automatically added to the Filament scene.
     *
     * @param meshData    CPU-side mesh data (positions, normals, indices).
     * @param material    Material handle from MaterialSystem.
     * @param castShadows Whether this renderable casts shadows.
     * @return Handle to the created renderable. Destroy with DestroyRenderable().
     */
    RenderableHandle CreateRenderable(const MeshData& meshData,
                                       const MaterialHandle& material,
                                       bool castShadows = true);

    /**
     * Destroy a previously created renderable and remove it from the scene.
     */
    void DestroyRenderable(RenderableHandle& handle);

    /**
     * Update the transform of a renderable entity.
     *
     * @param handle    The renderable to transform.
     * @param transform Column-major 4x4 transform matrix (glm::mat4 compatible).
     */
    void SetTransform(const RenderableHandle& handle, const float* transform);

    /**
     * Returns true only if this handle still references a live renderable slot.
     */
    bool IsAlive(const RenderableHandle& handle) const;

private:
    struct ManagedRenderable {
        utils::Entity          entity{};
        filament::VertexBuffer* vertexBuffer = nullptr;
        filament::IndexBuffer*  indexBuffer  = nullptr;
        uint32_t                generation   = 1;
        bool                    alive        = false;
    };

    filament::Engine* engine_ = nullptr;
    filament::Scene*  scene_  = nullptr;

    std::vector<ManagedRenderable> renderables_;
    std::vector<uint32_t>          free_slots_;
};

} // namespace se
