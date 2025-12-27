#pragma once

#include "engine/Log.h"
#include "engine/ecs/Component.h"
#include "engine/ecs/ComponentSystem.h"
#include "engine/ecs/ScriptComponent.h"

namespace se {

// ==================== Entity Lifecycle Component Template Implementations ====================

template <typename T, typename... Args>
T& Entity::AddLifecycleComponent(Args&&... args) {
    // Ensure ScriptComponent exists on the entity
    if (!HasComponent<ScriptComponent>()) {
        scene_->registry_.emplace<ScriptComponent>(entityHandle_);
    }

    auto& sc = GetComponent<ScriptComponent>();

    // Check if component already exists
    if (sc.Has<T>()) {
        SE_LOG_WARN("Entity already has component of type!");
        return *sc.Get<T>();
    }

    // Create the component
    auto script_ptr = std::make_shared<T>(std::forward<Args>(args)...);
    T& script_ref = *script_ptr;

    // Initialize internal state with entity ID and scene
    script_ptr->InitializeInternal(GetID(), scene_);

    // Store in ScriptComponent
    sc.components.push_back(script_ptr);

    // Register with ComponentSystem for lifecycle updates
    if (scene_->GetComponentSystem()) {
        scene_->GetComponentSystem()->RegisterComponent(script_ptr.get());
    }

    return script_ref;
}

template <typename T>
T* Entity::FindComponent() {
    static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");

    if (!HasComponent<ScriptComponent>()) { return nullptr; }

    auto& sc = GetComponent<ScriptComponent>();
    return sc.Get<T>();
}

template <typename T>
bool Entity::HasLifecycleComponent() {
    static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");

    if (!HasComponent<ScriptComponent>()) { return false; }

    auto& sc = GetComponent<ScriptComponent>();
    return sc.Has<T>();
}

template <typename T>
void Entity::RemoveLifecycleComponent() {
    static_assert(std::is_base_of_v<Component, T>, "T must inherit from Component");

    if (!HasComponent<ScriptComponent>()) {
        SE_LOG_WARN("Entity does not have any lifecycle components!");
        return;
    }

    auto& sc = GetComponent<ScriptComponent>();
    T* script = sc.Get<T>();

    if (!script) {
        SE_LOG_WARN("Entity does not have component of type!");
        return;
    }

    // Unregister from ComponentSystem
    if (scene_->GetComponentSystem()) {
        scene_->GetComponentSystem()->UnregisterComponent(script);
    }

    sc.Remove<T>();
}

// ==================== Component Template Implementations ====================

template <typename T>
T& Component::GetComponent() {
    Entity entity = GetEntity();
    return entity.GetComponent<T>();
}

template <typename T>
bool Component::HasComponent() {
    Entity entity = GetEntity();
    return entity.HasComponent<T>();
}

}  // namespace se
