#pragma once

#include <entt.hpp>
#include <memory>
#include <string>
#include <type_traits>

#include "engine/ecs/Entity.h"

// Forward declaration
class Camera;

namespace se {

class Component;
class ComponentSystem;
class PhysicsSystem;
struct ScriptComponent;
enum class CameraMode;

struct SceneSettings {
    bool EnablePhysics = true;
};

class Scene {
   public:
    Scene(const std::string& name = "Untitled Scene", const SceneSettings& settings = {});
    ~Scene();

    // Disable copy
    Scene(const Scene&)            = delete;
    Scene& operator=(const Scene&) = delete;

    // Allow move
    Scene(Scene&&)            = default;
    Scene& operator=(Scene&&) = default;

    Entity CreateEntity(const std::string& name = "Entity");
    void   DestroyEntity(Entity entity);

    // High-level entity creation helpers
    Entity CreateCamera(const std::string& name = "Camera", CameraMode mode = static_cast<CameraMode>(0));
    Entity CreateDirectionalLight(const std::string& name = "Sun", float intensity = 110000.0f);
    Entity CreatePointLight(const std::string& name = "Point Light", float intensity = 100000.0f);

    template <typename... Components>
    auto GetAllEntitiesWith() {
        return registry_.view<Components...>();
    }

    Entity FindEntityByName(const std::string& name);

    const std::string& GetName() const {
        return name_;
    }

    void OnUpdate(float deltaTime);
    
    // Render with explicit camera
    void OnRender(const Camera& camera, float aspectRatio);
    
    // Render using the active camera (if set)
    void OnRender();

    void Clear();

    size_t GetEntityCount() const {
        return registry_.storage<entt::entity>()->size();
    }

    // Active camera management
    void SetActiveCamera(Camera* camera) {
        active_camera_ = camera;
    }
    Camera* GetActiveCamera() {
        return active_camera_;
    }
    const Camera* GetActiveCamera() const {
        return active_camera_;
    }

    // Physics access - returns nullptr if physics not enabled
    PhysicsSystem* GetPhysicsSystem() {
        return physics_system_.get();
    }
    const PhysicsSystem* GetPhysicsSystem() const {
        return physics_system_.get();
    }
    bool HasPhysics() const {
        return physics_system_ != nullptr;
    }

    // Component system access (manages lifecycle of Component-derived scripts)
    ComponentSystem* GetComponentSystem() {
        return component_system_.get();
    }
    const ComponentSystem* GetComponentSystem() const {
        return component_system_.get();
    }

    entt::registry& GetRegistry() {
        return registry_;
    }
    const entt::registry& GetRegistry() const {
        return registry_;
    }

   private:
    void UpdateTransforms();
    
    std::string    name_;
    entt::registry registry_;
    Camera*        active_camera_ = nullptr;
    float          last_delta_time_ = 0.016f;

    // Systems owned by Scene
    std::unique_ptr<PhysicsSystem> physics_system_;
    std::unique_ptr<ComponentSystem> component_system_;

    friend class Entity;
    friend class RenderSystem;
};

}  // namespace se

// Include template implementations
#include "engine/ecs/SceneTemplates.h"