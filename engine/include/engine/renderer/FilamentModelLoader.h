#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "engine/renderer/ModelHandle.h"

namespace filament {
class Engine;
class Scene;
} // namespace filament

namespace filament::gltfio {
class AssetLoader;
class MaterialProvider;
class ResourceLoader;
class TextureProvider;
class FilamentAsset;
} // namespace filament::gltfio

namespace se {

/**
 * Loads glTF 2.0 (.gltf / .glb) models into the Filament scene using gltfio.
 *
 * Uses UbershaderProvider for PBR materials (precompiled, fast).
 * Handles resource loading (textures, buffers) automatically.
 *
 * Thread Safety: Not thread-safe. Must be called from the main thread.
 */
class FilamentModelLoader {
public:
    FilamentModelLoader() = default;
    ~FilamentModelLoader();

    // Non-copyable
    FilamentModelLoader(const FilamentModelLoader&) = delete;
    FilamentModelLoader& operator=(const FilamentModelLoader&) = delete;

    /**
     * Initialize with Filament engine and scene.
     */
    void Init(filament::Engine* engine, filament::Scene* scene);

    /**
     * Shutdown and destroy all loaded models.
     */
    void Shutdown();

    /**
     * Load a .glb or .gltf model from disk.
     * The model's renderables are automatically added to the Filament scene.
     *
     * @param path Path to the .glb or .gltf file.
     * @return Handle to the loaded model. Destroy with DestroyModel().
     */
    ModelHandle LoadModel(const std::string& path);

    /**
     * Destroy a previously loaded model and remove it from the scene.
     */
    void DestroyModel(ModelHandle& handle);

    /**
     * Set the root transform of a loaded model.
     *
     * @param handle    The model to transform.
     * @param transform Column-major 4x4 transform matrix.
     */
    void SetTransform(const ModelHandle& handle, const float* transform);

    /**
     * Update all model animations by a time delta.
     * Call this every frame to advance skeletal animations.
     *
     * @param deltaTime Time since last frame in seconds.
     */
    void UpdateAnimations(float deltaTime);

private:
    filament::Engine* engine_ = nullptr;
    filament::Scene*  scene_  = nullptr;

    filament::gltfio::AssetLoader*      asset_loader_     = nullptr;
    filament::gltfio::MaterialProvider*  material_provider_ = nullptr;
    filament::gltfio::ResourceLoader*    resource_loader_   = nullptr;
    filament::gltfio::TextureProvider*   texture_provider_  = nullptr;

    // All loaded assets (owned by this system)
    std::vector<filament::gltfio::FilamentAsset*> assets_;
    std::unordered_map<filament::gltfio::FilamentAsset*, float> animation_times_;
};

} // namespace se
