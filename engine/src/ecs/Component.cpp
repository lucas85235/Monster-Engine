#include "engine/ecs/Component.h"

#include "engine/core/Log.h"
#include "engine/ecs/Entity.h"
#include "engine/ecs/Scene.h"

namespace se {

void Component::SetEnabled(bool enabled) {
    if (enabled_ == enabled) return;

    enabled_ = enabled;
    if (enabled_) {
        OnEnable();
    } else {
        OnDisable();
    }
}

Entity Component::GetEntity() const {
    if (!scene_) { return Entity(); }
    
    // Reconstruct Entity from stored ID and Scene
    entt::entity handle = static_cast<entt::entity>(owner_entity_id_);
    return Entity(handle, scene_);
}

void Component::InitializeInternal(uint32_t entity_id, Scene* scene) {
    owner_entity_id_ = entity_id;
    scene_           = scene;
    
    SE_LOG_INFO("Component initialized for entity {}", entity_id);
    Awake();
}

}  // namespace se
