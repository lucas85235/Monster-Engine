#pragma once

#include <vector>

namespace se {

class Component;
class Scene;

/**
 * System that manages the lifecycle of all Component instances with lifecycle.
 * Handles Start/Update/FixedUpdate/LateUpdate calls in the correct order.
 */
class ComponentSystem {
   public:
    explicit ComponentSystem(Scene* scene);
    ~ComponentSystem();

    ComponentSystem(const ComponentSystem&)            = delete;
    ComponentSystem& operator=(const ComponentSystem&) = delete;

    void ProcessPendingStarts();

    void Update(float dt);

    void FixedUpdate(float dt);
    
    // Run FixedUpdate exactly once with the given dt (no accumulator)
    // Used by Bullet physics tick callback for perfect sync
    void RunFixedUpdateOnce(float dt);

    void LateUpdate(float dt);

    void RegisterComponent(Component* component);

    void UnregisterComponent(Component* component);

    size_t GetActiveComponentCount() const {
        return active_components_.size();
    }

    size_t GetPendingStartCount() const {
        return pending_start_.size();
    }

   private:
    Scene*                   scene_;
    std::vector<Component*>  pending_start_;
    std::vector<Component*>  active_components_;
    float                    fixed_time_accumulator_ = 0.0f;
    static constexpr float   FIXED_TIMESTEP          = 1.0f / 60.0f;
    bool                     is_updating_            = false;
    std::vector<Component*>  pending_register_;
    std::vector<Component*>  pending_unregister_;
};

}  // namespace se
