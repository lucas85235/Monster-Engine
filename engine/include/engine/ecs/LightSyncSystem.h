#pragma once

namespace se {

class Scene;

/**
 * Synchronizes ECS light components (DirectionalLightComponent, PointLightComponent)
 * with the Filament LightSystem each frame.
 *
 * Reads TransformComponent rotation for directional light direction,
 * and TransformComponent position for point light placement.
 */
class LightSyncSystem {
   public:
    /**
     * Sync all light entities in the scene to the Filament LightSystem.
     * Should be called once per frame, typically during Scene::OnRender.
     */
    static void Sync(Scene& scene);

   private:
    LightSyncSystem() = delete;
};

}  // namespace se
