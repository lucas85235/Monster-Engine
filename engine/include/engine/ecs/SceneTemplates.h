#pragma once

// This file contains Entity template implementations that need Scene to be fully defined
// It is included at the end of Scene.h

#include "engine/core/Log.h"
#include "engine/ecs/Component.h"

namespace se {

// ==================== Entity AddComponent with if constexpr ====================

template <typename T, typename... Args>
T& Entity::AddComponent(Args&&... args) {
    if constexpr (std::is_base_of_v<Component, T>) {
        return AddLifecycleComponent<T>(std::forward<Args>(args)...);
    } else {
        return AddDataComponent<T>(std::forward<Args>(args)...);
    }
}

// ==================== Entity Data Component Helpers ====================

template <typename T, typename... Args>
T& Entity::AddDataComponent(Args&&... args) {
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
#include "engine/ecs/SceneScriptTemplates.h"
