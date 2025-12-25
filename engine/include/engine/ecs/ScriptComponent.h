#pragma once

#include <memory>
#include <vector>

namespace se {

class Component;

/**
 * ECS component that holds all Component instances (with lifecycle) for an entity.
 * This is stored in EnTT registry like any other component,
 * bridging the gap between pure ECS data components and OOP-style components with lifecycle.
 */
struct ScriptComponent {
    std::vector<std::shared_ptr<Component>> components;

    template <typename T>
    T* Get() {
        for (auto& c : components) {
            if (auto* ptr = dynamic_cast<T*>(c.get())) { return ptr; }
        }
        return nullptr;
    }

    template <typename T>
    const T* Get() const {
        for (const auto& c : components) {
            if (const auto* ptr = dynamic_cast<const T*>(c.get())) { return ptr; }
        }
        return nullptr;
    }

    template <typename T>
    bool Has() const {
        for (const auto& c : components) {
            if (dynamic_cast<const T*>(c.get())) { return true; }
        }
        return false;
    }

    template <typename T>
    bool Remove() {
        for (auto it = components.begin(); it != components.end(); ++it) {
            if (dynamic_cast<T*>(it->get())) {
                components.erase(it);
                return true;
            }
        }
        return false;
    }
};

}  // namespace se
