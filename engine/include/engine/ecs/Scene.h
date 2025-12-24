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

    template <typename... Components>
    auto GetAllEntitiesWith() {
        return registry_.view<Components...>();
    }

    Entity FindEntityByName(const std::string& name);

    const std::string& GetName() const {
        return name_;
    }

    void OnUpdate(float deltaTime);
    void OnRender(const Camera& camera, float aspectRatio);

    void Clear();

    size_t GetEntityCount() const {
        return registry_.storage<entt::entity>()->size();
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
    std::string    name_;
    entt::registry registry_;

    // Systems owned by Scene
    std::unique_ptr<PhysicsSystem> physics_system_;
    std::unique_ptr<ComponentSystem> component_system_;

    friend class Entity;
    friend class RenderSystem;
};

}  // namespace se

// Include template implementations
#include "engine/ecs/SceneTemplates.h"