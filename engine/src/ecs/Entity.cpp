#include "engine/ecs/Entity.h"

#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/Log.h"
#include <algorithm>

namespace se {

void Entity::SetParent(Entity parent) {
    if (!IsValid()) {
        SE_LOG_WARN("Cannot SetParent on invalid entity");
        return;
    }

    // Check circular dependency? (Skipped for simplicity, be careful)
    
    // Check if parent is valid and in same scene (if parent is not null)
    if (parent && (parent.GetScene() != scene_)) {
        SE_LOG_ERROR("Cannot set parent from a different scene");
        return;
    }

    // Get or create relationship component for this entity
    if (!HasComponent<RelationshipComponent>()) {
        AddComponent<RelationshipComponent>();
    }
    auto& rel = GetComponent<RelationshipComponent>();

    // If current parent is same as new parent, do nothing
    if ((entt::entity)parent == rel.Parent) {
        return;
    }

    // 1. Unlink from old parent
    if (rel.Parent != entt::null) {
        Entity oldParent(rel.Parent, scene_);
        if (oldParent.IsValid() && oldParent.HasComponent<RelationshipComponent>()) {
            auto& oldChildren = oldParent.GetComponent<RelationshipComponent>().Children;
            oldChildren.erase(std::remove(oldChildren.begin(), oldChildren.end(), entityHandle_), oldChildren.end());
        }
    }

    // 2. Set new parent
    rel.Parent = (entt::entity)parent;

    // 3. Link to new parent
    if (parent) {
        if (!parent.HasComponent<RelationshipComponent>()) {
            parent.AddComponent<RelationshipComponent>();
        }
        parent.GetComponent<RelationshipComponent>().Children.push_back(entityHandle_);
    }
}

Entity Entity::GetParent() const {
    if (!IsValid() || !HasComponent<RelationshipComponent>()) {
        return Entity();
    }
    return Entity(GetComponent<RelationshipComponent>().Parent, scene_);
}

std::vector<Entity> Entity::GetChildren() const {
    std::vector<Entity> children;
    if (!IsValid() || !HasComponent<RelationshipComponent>()) {
        return children;
    }

    const auto& rel = GetComponent<RelationshipComponent>();
    children.reserve(rel.Children.size());
    for (auto handle : rel.Children) {
        children.emplace_back(handle, scene_);
    }
    return children;
}

bool Entity::HasParent() const {
    if (!IsValid() || !HasComponent<RelationshipComponent>()) return false;
    return GetComponent<RelationshipComponent>().Parent != entt::null;
}

bool Entity::HasChildren() const {
    if (!IsValid() || !HasComponent<RelationshipComponent>()) return false;
    return !GetComponent<RelationshipComponent>().Children.empty();
}



}  // namespace se