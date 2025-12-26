#pragma once

#include "se_pch.h"
#include "engine/core/CoreTypes.h"
#include "LinearMath/btVector3.h"

namespace se {

// Bullet Physics conversion utilities
[[nodiscard]] inline glm::vec3 ToGlm(const btVector3& v) noexcept {
    return glm::vec3{
        static_cast<float>(v.getX()),
        static_cast<float>(v.getY()),
        static_cast<float>(v.getZ())
    };
}

[[nodiscard]] inline btVector3 ToBt(const glm::vec3& v) noexcept {
    return btVector3{
        static_cast<btScalar>(v.x),
        static_cast<btScalar>(v.y),
        static_cast<btScalar>(v.z)
    };
}

// Event type ID generation
namespace detail {
inline EventTypeId GenerateTypeId() {
    static EventTypeId counter = 0;
    return counter++;
}

template <typename T>
EventTypeId GetTypeId() {
    static EventTypeId id = GenerateTypeId();
    return id;
}
} // namespace detail

} // namespace se

// ECS includes - must come after se namespace types are defined
#include "engine/ecs/Component.h"
#include "engine/ecs/Entity.h"
#include "engine/ecs/Scene.h"

// Core utilities
#include "engine/core/Time.h"
