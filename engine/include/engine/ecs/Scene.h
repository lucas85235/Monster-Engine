
#pragma once

#include <entt.hpp>
#include <string>

#include "engine/Camera.h"
#include "engine/Log.h"
#include "engine/ecs/Entity.h"

namespace se {

class Scene {
   public:
    Scene(const std::string& name = "Untitled Scene");
    ~Scene();

    // Create a new entity
    Entity CreateEntity(const std::string& name = "Entity");

    // Destroy an entity
    void DestroyEntity(Entity entity);

    // Get all entities with specific components
    template <typename... Components>
    auto GetAllEntitiesWith() {
        return registry_.view<Components...>();
    }

    // Find entity by name
    Entity FindEntityByName(const std::string& name);

    // Get scene name
    const std::string& GetName() const {
        return name_;
    }

    // Update scene (can be used for systems)
    void OnUpdate(float deltaTime);

    // Render scene (automatically renders all MeshRenderComponents)
    void OnRender(const Camera& camera, float aspectRatio);

    // Clear all entities
    void Clear();

    // Get entity count (number of alive entities)
    size_t GetEntityCount() const {
        return registry_.storage<entt::entity>()->size();
    }

   private:
    std::string    name_;
    entt::registry registry_;

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