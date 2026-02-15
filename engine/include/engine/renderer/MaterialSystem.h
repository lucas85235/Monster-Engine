#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "engine/renderer/MaterialHandle.h"
#include "engine/renderer/TextureHandle.h"

namespace filament {
class Engine;
class Material;
class MaterialInstance;
class Texture;
class TextureSampler;
} // namespace filament

namespace se {

/**
 * Configuration for creating a material instance.
 * Supports both uniform-only and texture-mapped PBR workflows.
 */
struct MaterialConfig {
    float baseColor[4]  = {0.8f, 0.8f, 0.8f, 1.0f}; // Linear RGBA
    float metallic      = 0.0f;
    float roughness     = 0.5f;
    float reflectance   = 0.5f;
    float emissive[3]   = {0.0f, 0.0f, 0.0f};
    float emissiveIntensity = 0.0f;

    // Optional texture maps — leave invalid for uniform-only mode
    TextureHandle baseColorMap;
    TextureHandle normalMap;
    TextureHandle metallicRoughnessMap;
    TextureHandle aoMap;
};

/**
 * Manages Filament Material and MaterialInstance lifetimes.
 *
 * Provides built-in materials for common use cases and allows
 * creating custom material instances with PBR parameters.
 *
 * All Materials are owned by this system and destroyed on Shutdown().
 */
class MaterialSystem {
public:
    MaterialSystem() = default;
    ~MaterialSystem();

    // Non-copyable
    MaterialSystem(const MaterialSystem&) = delete;
    MaterialSystem& operator=(const MaterialSystem&) = delete;

    /**
     * Initialize with a Filament engine. Must be called before any other method.
     */
    void Init(filament::Engine* engine);

    /**
     * Shutdown and destroy all materials.
     */
    void Shutdown();

    /**
     * Create a new PBR material instance with the given configuration.
     */
    MaterialHandle CreateMaterial(const MaterialConfig& config = {});

    /**
     * Get the default lit material (white, roughness=0.5, non-metallic).
     */
    MaterialHandle GetDefaultLit();

    /**
     * Get a flat unlit material (no lighting).
     */
    MaterialHandle GetDefaultUnlit();

    /**
     * Get the underlying Filament Material for advanced use.
     */
    filament::Material* GetLitMaterial() const { return lit_material_; }

private:
    friend class MaterialHandle;

    struct MaterialSlot {
        filament::MaterialInstance* instance   = nullptr;
        uint32_t                    generation = 1;
        bool                        alive      = false;
    };

    void CreateBuiltInMaterials();
    void EnsureFallbackTextures();
    MaterialHandle AddInstance(filament::MaterialInstance* instance);
    filament::MaterialInstance* Resolve(const MaterialHandle& handle) const;
    bool IsAlive(const MaterialHandle& handle) const;

    filament::Engine*   engine_ = nullptr;

    // Built-in Filament Materials (template materials)
    filament::Material* lit_material_   = nullptr;
    filament::Material* unlit_material_ = nullptr;

    // Internal fallback textures to guarantee all required samplers are always bound.
    filament::Texture* fallback_white_texture_  = nullptr;
    filament::Texture* fallback_normal_texture_ = nullptr;
    filament::Texture* fallback_black_texture_  = nullptr;

    // All created instances (owned by this system), tracked via generation slots.
    std::vector<MaterialSlot> material_slots_;
    std::vector<uint32_t>     free_slots_;

    // Default instances
    MaterialHandle default_lit_;
    MaterialHandle default_unlit_;
};

} // namespace se
