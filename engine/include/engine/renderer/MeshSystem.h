#pragma once

#include <unordered_map>
#include <vector>

#include "engine/renderer/MaterialHandle.h"

namespace filament {
class Engine;
class Scene;
class VertexBuffer;
class IndexBuffer;
} // namespace filament

namespace utils {
class Entity;
} // namespace utils

namespace se {

struct MeshData;

/**
 * Result of creating a Filament renderable from MeshData.
 *
 * The renderable entity is already added to the Filament scene.
 * Destroy via MeshSystem::DestroyRenderable().
 */
struct RenderableHandle {
    utils::Entity* filamentEntity = nullptr; // Filament entity (heap-allocated for ABI safety)
    bool IsValid() const { return filamentEntity != nullptr; }
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

private:
    struct ManagedRenderable {
        utils::Entity*         entity       = nullptr;
        filament::VertexBuffer* vertexBuffer = nullptr;
        filament::IndexBuffer*  indexBuffer  = nullptr;
    };

    filament::Engine* engine_ = nullptr;
    filament::Scene*  scene_  = nullptr;

    std::vector<ManagedRenderable> renderables_;
};

} // namespace se
