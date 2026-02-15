#pragma once

#include <vector>
#include <utils/Entity.h>

namespace filament {
class Engine;
class Scene;
class LightManager;
} // namespace filament

namespace se {

/**
 * Manages Filament lights in the scene.
 *
 * Creates and updates directional, point, and spot lights
 * using Filament's LightManager.
 */
class LightSystem {
public:
    LightSystem() = default;
    ~LightSystem();

    // Non-copyable
    LightSystem(const LightSystem&) = delete;
    LightSystem& operator=(const LightSystem&) = delete;

    /**
     * Initialize with Filament engine and scene.
     */
    void Init(filament::Engine* engine, filament::Scene* scene);

    /**
     * Shutdown and destroy all managed lights.
     */
    void Shutdown();

    /**
     * Create a directional light (like the sun).
     *
     * @param dirX, dirY, dirZ Direction vector (does not need to be normalized).
     * @param r, g, b           Color in linear RGB.
     * @param intensity         Light intensity in lux (default: 100000 for outdoor sun).
     * @param castShadows       Whether this light casts shadows.
     */
    void SetDirectionalLight(float dirX, float dirY, float dirZ,
                              float r, float g, float b,
                              float intensity = 100000.0f,
                              bool castShadows = true);

    /**
     * Create a point light at a world position.
     *
     * @return Index of the created light (for removal).
     */
    size_t AddPointLight(float posX, float posY, float posZ,
                         float r, float g, float b,
                         float intensity = 100000.0f,
                         float falloff = 10.0f);

    /**
     * Remove all lights from the scene.
     */
    void ClearLights();

private:
    filament::Engine* engine_ = nullptr;
    filament::Scene*  scene_  = nullptr;

    // Directional light entity (only one at a time)
    utils::Entity directional_light_{};
    bool          has_directional_light_ = false;

    // Point/spot light entities
    std::vector<utils::Entity> point_lights_;
};

} // namespace se
