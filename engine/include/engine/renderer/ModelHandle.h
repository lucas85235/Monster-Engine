#pragma once

#include <cstdint>

namespace filament::gltfio {
class FilamentAsset;
class Animator;
} // namespace filament::gltfio

namespace utils {
class Entity;
} // namespace utils

namespace se {

/**
 * Opaque handle to a loaded glTF/glB model via Filament's gltfio.
 *
 * Wraps a gltfio::FilamentAsset and provides access to the root entity
 * (for transforms) and the Animator (for skeletal animation).
 *
 * Lifetime: owned by FilamentModelLoader. Handle becomes invalid if the
 * FilamentModelLoader destroys the underlying asset.
 */
class ModelHandle {
public:
    ModelHandle() = default;

    /** Check if handle points to a valid loaded model. */
    bool IsValid() const { return asset_ != nullptr; }
    explicit operator bool() const { return IsValid(); }

    /** Get the root entity for applying transforms. */
    utils::Entity GetRoot() const;

    /** Get the number of renderable entities in this model. */
    size_t GetEntityCount() const;

    /** Get the animator for skeletal animation (may be null if no animations). */
    filament::gltfio::Animator* GetAnimator() const;

    /** Direct access to the underlying FilamentAsset for advanced use. */
    filament::gltfio::FilamentAsset* GetNative() const { return asset_; }

private:
    friend class FilamentModelLoader;
    explicit ModelHandle(filament::gltfio::FilamentAsset* asset)
        : asset_(asset) {}

    filament::gltfio::FilamentAsset* asset_ = nullptr;
};

} // namespace se
