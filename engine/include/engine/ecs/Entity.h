#pragma once

#include <cstdint>
#include <entt.hpp>

namespace se {

// Forward declaration
class Scene;

class Entity {
  public:
    Entity() = default;
    Entity(entt::entity handle, Scene* scene);
    Entity(const Entity& other) = default;

    // Add component to entity
    template <typename T, typename... Args>
    T& AddComponent(Args&&... args);

    // Get component from entity
    template <typename T>
    T& GetComponent();

    // Check if entity has component
    template <typename T>
    bool HasComponent();

    // Remove component from entity
    template <typename T>
    void RemoveComponent();

    // Get entity ID
    uint32_t GetID() const {
        return static_cast<uint32_t>(entityHandle_);
    }

    // Check if entity is valid
    bool IsValid() const {
        return entityHandle_ != entt::null && scene_ != nullptr;
    }

    // Comparison operators
    bool operator==(const Entity& other) const {
        return entityHandle_ == other.entityHandle_ && scene_ == other.scene_;
    }

    bool operator!=(const Entity& other) const {
        return !(*this == other);
    }

    // Cast to bool
    operator bool() const {
        return IsValid();
    }

    // Cast to entt::entity
    operator entt::entity() const {
        return entityHandle_;
    }

    // Get underlying handle
    entt::entity GetHandle() const {
        return entityHandle_;
    }

  private:
    entt::entity entityHandle_{entt::null};
    Scene* scene_ = nullptr;

    friend class Scene;
};

} // namespace se