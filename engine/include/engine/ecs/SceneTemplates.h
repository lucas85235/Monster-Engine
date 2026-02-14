#pragma once

// This file contains Entity template implementations that need Scene to be fully defined
// It is included at the end of Scene.h

#include "engine/Log.h"
#include "engine/ecs/Component.h"
#include "engine/ecs/ScriptComponent.h"

#include <stdexcept>
#include <typeinfo>

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
const T& Entity::GetComponent() const {
    if (!scene_ || entityHandle_ == entt::null) {
        SE_LOG_ERROR("GetComponent called on invalid entity");
        throw std::runtime_error("GetComponent on invalid entity");
    }

    if constexpr (std::is_base_of_v<Component, T>) {
        if (const auto* script = scene_->registry_.try_get<ScriptComponent>(entityHandle_)) {
            if (const auto* component = script->Get<T>()) {
                return *component;
            }
        }
    } else {
        if (scene_->registry_.all_of<T>(entityHandle_)) {
            return scene_->registry_.get<T>(entityHandle_);
        }
    }

    SE_LOG_ERROR("Entity {} does not have component type {}", GetID(), typeid(T).name());
    throw std::runtime_error("Entity missing requested component");
}

 template <typename T>
T& Entity::GetComponent() {
    if (!scene_ || entityHandle_ == entt::null) {
        SE_LOG_ERROR("GetComponent called on invalid entity");
        throw std::runtime_error("GetComponent on invalid entity");
    }

    if constexpr (std::is_base_of_v<Component, T>) {
        if (auto* script = scene_->registry_.try_get<ScriptComponent>(entityHandle_)) {
            if (auto* component = script->Get<T>()) {
                return *component;
            }
        }
    } else {
        if (scene_->registry_.all_of<T>(entityHandle_)) {
            return scene_->registry_.get<T>(entityHandle_);
        }
    }

    SE_LOG_ERROR("Entity {} does not have component type {}", GetID(), typeid(T).name());
    throw std::runtime_error("Entity missing requested component");
}

template <typename T>
bool Entity::HasComponent() const {
    if (!scene_ || entityHandle_ == entt::null) {
        return false;
    }

    if constexpr (std::is_base_of_v<Component, T>) {
        if (const auto* script = scene_->registry_.try_get<ScriptComponent>(entityHandle_)) {
            return script->Has<T>();
        }
        return false;
    } else {
        return scene_->registry_.all_of<T>(entityHandle_);
    }
}

template <typename T>
void Entity::RemoveComponent() {
    if constexpr (std::is_base_of_v<Component, T>) {
        RemoveLifecycleComponent<T>();
    } else {
        if (!HasComponent<T>()) {
            SE_LOG_WARN("Entity does not have component!");
            return;
        }
        scene_->registry_.remove<T>(entityHandle_);
    }
}

}  // namespace se

// Include lifecycle component template implementations
#include "engine/ecs/SceneScriptTemplates.h"
