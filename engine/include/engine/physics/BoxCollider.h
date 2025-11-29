#pragma once
#include <btBulletCollisionCommon.h>

#include "engine/ecs/Component.h"

namespace se {
struct BoxCollider {
    Vector3 Size = {1.0f, 1.0f, 1.0f};
    Vector3 Offset = {0.0f, 0.0f, 0.0f};

    BoxCollider() = default;
    BoxCollider(const BoxCollider&) = default;
    BoxCollider(const Vector3& size) : Size(size) {}
};

struct SphereCollider {
    float Radius = 0.5f;
    Vector3 Offset = {0.0f, 0.0f, 0.0f};

    SphereCollider() = default;
    SphereCollider(const SphereCollider&) = default;
    SphereCollider(float radius) : Radius(radius) {}
};

struct CapsuleCollider {
    float Radius = 0.5f;
    float Height = 1.0f;
    Vector3 Offset = {0.0f, 0.0f, 0.0f};

    CapsuleCollider() = default;
    CapsuleCollider(const CapsuleCollider&) = default;
    CapsuleCollider(float radius, float height) : Radius(radius), Height(height) {}
};
}  // namespace se