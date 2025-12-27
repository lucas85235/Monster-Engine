#pragma once

#include <cstdint>
#include <entt.hpp>
#include <type_traits>

namespace se {

// Forward declarations
class Scene;
class Component;

class Entity {
   public:
    Entity() = default;
    Entity(entt::entity handle, Scene* scene) : entityHandle_(handle), scene_(scene) {}
    Entity(const Entity& other) = default;
    Entity& operator=(const Entity& other) = default;

    /**
     * Add a component to entity.
     * If T inherits from Component, the lifecycle system is used (Awake/Start/Update called automatically).
     * Otherwise, it's added as a pure ECS data component.
     */
    template <typename T, typename... Args>
    T& AddComponent(Args&&... args);

    // Get component from entity
    template <typename T>
    T& GetComponent();

    template <typename T>
    const T& GetComponent() const;

    // Check if entity has component
    template <typename T>
    bool HasComponent() const;

    // Remove component from entity
    template <typename T>
    void RemoveComponent();

    // Find a lifecycle-managed component by type (returns nullptr if not found)
    template <typename T>
    T* FindComponent();

    // Check if entity has a specific lifecycle-managed component
    template <typename T>
    bool HasLifecycleComponent();

    // Remove a lifecycle-managed component by type
    template <typename T>
    void RemoveLifecycleComponent();

    // ==================== Hierarchy API ====================
    // Set parent entity
    void SetParent(Entity parent);
    
    // Get parent entity
    Entity GetParent() const;
    
    // Get all children
    std::vector<Entity> GetChildren() const;
    
    // Check if entity has parent
    bool HasParent() const;
    
    // Check if entity has children
    bool HasChildren() const;

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

    Scene* GetScene() const {
        return scene_;
    }

   private:
    entt::entity entityHandle_{entt::null};
    Scene*       scene_ = nullptr;

    // Internal helper for adding lifecycle components
    template <typename T, typename... Args>
    T& AddLifecycleComponent(Args&&... args);

    // Internal helper for adding data components
    template <typename T, typename... Args>
    T& AddDataComponent(Args&&... args);

    friend class Scene;
};

}  // namespace se