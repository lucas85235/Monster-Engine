#pragma once

#include <entt.hpp>
#include <memory>
#include <string>

#include "engine/renderer/Camera.h"
#include "engine/core/Log.h"
#include "engine/ecs/Entity.h"

namespace se {

class PhysicsSystem;
class ComponentSystem;

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

    // View access
    template <typename... Components>
    auto GetAllEntitiesWith() {
        return registry_.view<Components...>();
    }

    template <typename... Components>
    auto GetAllEntitiesWith() const {
        return registry_.view<const Components...>();
    }

    Entity FindEntityByName(const std::string& name);

    const std::string& GetName() const {
        return name_;
    }

    void OnUpdate(float deltaTime);
    void OnRender(const Camera& camera, float aspectRatio);
    void OnRender();

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

    // Component system access
    ComponentSystem* GetComponentSystem() {
        return component_system_.get();
    }

    // Camera management
    void SetActiveCamera(Camera* camera) {
        active_camera_ = camera;
    }
    Camera* GetActiveCamera() {
        return active_camera_;
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

    // Physics is optional and owned by Scene via unique_ptr
    std::unique_ptr<PhysicsSystem> physics_system_;
    
    // Component system for lifecycle management
    std::unique_ptr<ComponentSystem> component_system_;
    
    // Active camera for rendering (not owned)
    Camera* active_camera_ = nullptr;

    friend class Entity;
    friend class RenderSystem;
};

// ==================== Entity Template Implementations ====================

template <typename T, typename... Args>
T& Entity::AddComponent(Args&&... args) {
    if (HasComponent<T>()) {
        SE_LOG_WARN("Entity already has component!");
        return GetComponent<T>();
    }
    return scene_->registry_.emplace<T>(entityHandle_, std::forward<Args>(args)...);
}

template <typename T>
T& Entity::GetComponent() {
    if (!HasComponent<T>()) { SE_LOG_ERROR("Entity does not have component!"); }
    return scene_->registry_.get<T>(entityHandle_);
}

template <typename T>
bool Entity::HasComponent() {
    return scene_->registry_.all_of<T>(entityHandle_);
}

template <typename T>
void Entity::RemoveComponent() {
    if (!HasComponent<T>()) {
        SE_LOG_WARN("Entity does not have component!");
        return;
    }
    scene_->registry_.remove<T>(entityHandle_);
}

}  // namespace se

// Include lifecycle component template implementations
// This MUST come after Scene class is fully defined
#include "engine/ecs/SceneScriptTemplates.h"