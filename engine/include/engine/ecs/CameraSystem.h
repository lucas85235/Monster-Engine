#pragma once

namespace se {

class Scene;

/**
 * Processes CameraComponent + SpringArmComponent + TransformComponent entities
 * and syncs the active camera to Filament each frame.
 *
 * Handles camera input (mouse look, WASD movement) and mode-specific behavior:
 *   - FreeFly: direct position + rotation control via WASD + mouse
 *   - ThirdPerson: spring arm following a target entity with collision
 *   - Orbit: orbit around a target point
 *   - Fixed: no input processing, camera stays where placed
 */
class CameraSystem {
   public:
    /**
     * Update all camera entities in the scene.
     * Must be called once per frame, typically during Scene::OnRender.
     *
     * @param scene   The scene containing camera entities.
     * @param dt      Frame delta time in seconds.
     */
    static void Update(Scene& scene, float dt);

   private:
    CameraSystem() = delete;
};

}  // namespace se
