#pragma once

#include <cstdint>
#include <limits>

#include "engine/renderer/TextureHandle.h"

namespace filament {
class MaterialInstance;
} // namespace filament

namespace se {

class MaterialSystem;

/**
 * Opaque handle to a Filament MaterialInstance.
 *
 * Engine/game code uses this instead of touching filament::MaterialInstance directly.
 * Provides type-safe setters for common PBR parameters and texture maps.
 *
 * Lifetime: owned by MaterialSystem. Handle becomes invalid if the
 * MaterialSystem destroys the underlying instance.
 */
class MaterialHandle {
public:
    MaterialHandle() = default;

    /** Check if handle points to a valid material instance. */
    bool IsValid() const;
    explicit operator bool() const { return IsValid(); }

    /** Set base color (linear RGBA). */
    void SetColor(float r, float g, float b, float a = 1.0f);

    /** Set metallic factor [0, 1]. */
    void SetMetallic(float metallic);

    /** Set roughness factor [0, 1]. */
    void SetRoughness(float roughness);

    /** Set reflectance for dielectric materials [0, 1]. Default 0.5 = 4% F0. */
    void SetReflectance(float reflectance);

    /** Set emissive color (linear RGB) and intensity. */
    void SetEmissive(float r, float g, float b, float intensity = 1.0f);

    /** Set base color texture map (sRGB). Overrides uniform baseColor. */
    void SetBaseColorMap(const TextureHandle& texture);

    /** Set normal map (linear, tangent-space). */
    void SetNormalMap(const TextureHandle& texture);

    /** Set metallic-roughness map (linear). R=unused, G=roughness, B=metallic. */
    void SetMetallicRoughnessMap(const TextureHandle& texture);

    /** Set ambient occlusion map (linear, single-channel). */
    void SetAOMap(const TextureHandle& texture);

    /** Direct access for advanced use (e.g., setting textures via Filament API). */
    filament::MaterialInstance* GetNative() const;

private:
    friend class MaterialSystem;
    static constexpr uint32_t kInvalidIndex = std::numeric_limits<uint32_t>::max();

    MaterialHandle(MaterialSystem* owner, uint32_t index, uint32_t generation)
        : owner_(owner), index_(index), generation_(generation) {}

    MaterialSystem* owner_      = nullptr;
    uint32_t        index_      = kInvalidIndex;
    uint32_t        generation_ = 0;
};

} // namespace se
