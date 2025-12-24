#pragma once

#include <cstdint>

namespace se {

class Scene;
class ComponentSystem;
class Entity;

/**
 * Base class for components that require lifecycle management (similar to Unity's MonoBehaviour).
 *
 * Lifecycle order:
 *   1. Awake()      - Called immediately when the component is added to an entity
 *   2. OnEnable()   - Called when the component becomes enabled
 *   3. Start()      - Called once before the first Update, after all Awake calls
 *   4. FixedUpdate()- Called at fixed timestep intervals (physics)
 *   5. Update()     - Called every frame
 *   6. LateUpdate() - Called every frame after all Update calls
 *   7. OnDisable()  - Called when the component becomes disabled
 *   8. OnDestroy()  - Called when the component is removed or entity destroyed
 */
class Component {
   public:
    virtual ~Component() = default;

    virtual void Awake() {}
    virtual void Start() {}
    virtual void Update(float dt) {}
    virtual void FixedUpdate(float dt) {}
    virtual void LateUpdate(float dt) {}
    virtual void OnEnable() {}
    virtual void OnDisable() {}
    virtual void OnDestroy() {}

    bool IsEnabled() const {
        return enabled_;
    }

    void SetEnabled(bool enabled);

    bool HasStarted() const {
        return has_started_;
    }

    Entity GetEntity() const;
    
    uint32_t GetEntityID() const {
        return owner_entity_id_;
    }

    Scene* GetScene() const {
        return scene_;
    }

    template <typename T>
    T& GetComponent();

    template <typename T>
    bool HasComponent();

   protected:
    uint32_t owner_entity_id_ = 0;
    Scene*   scene_           = nullptr;
    bool     enabled_         = true;
    bool     has_started_     = false;

   private:
    friend class Scene;
    friend class ComponentSystem;
    friend class Entity;

    void InitializeInternal(uint32_t entity_id, Scene* scene);

    void MarkAsStarted() {
        has_started_ = true;
    }
};

}  // namespace se
