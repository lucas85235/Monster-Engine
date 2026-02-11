#pragma once

#include <cstdint>

namespace filament {
class MaterialInstance;
} // namespace filament

namespace se {

/**
 * Opaque handle to a Filament MaterialInstance.
 *
 * Engine/game code uses this instead of touching filament::MaterialInstance directly.
 * Provides type-safe setters for common PBR parameters.
 *
 * Lifetime: owned by MaterialSystem. Handle becomes invalid if the
 * MaterialSystem destroys the underlying instance.
 */
class MaterialHandle {
public:
    MaterialHandle() = default;

    /** Check if handle points to a valid material instance. */
    bool IsValid() const { return instance_ != nullptr; }
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

    /** Direct access for advanced use (e.g., setting textures via Filament API). */
    filament::MaterialInstance* GetNative() const { return instance_; }

private:
    friend class MaterialSystem;
    explicit MaterialHandle(filament::MaterialInstance* instance)
        : instance_(instance) {}

    filament::MaterialInstance* instance_ = nullptr;
};

} // namespace se
