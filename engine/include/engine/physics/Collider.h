#pragma once
#include <btBulletCollisionCommon.h>
#include <cstdint>
#include "engine/ecs/Component.h"
#include "Engine.h"

namespace se {
struct ColliderBase {
    uint16_t CollisionGroup = 0x0001;
    uint16_t CollisionMask  = 0xFFFF;
    bool     IsTrigger      = false;
};

struct BoxCollider : ColliderBase {
    Vector3 Size   = {1.0f, 1.0f, 1.0f};
    Vector3 Offset = {0.0f, 0.0f, 0.0f};

    BoxCollider() = default;

    BoxCollider(const BoxCollider&) = default;

    BoxCollider(const Vector3& size) : Size(size) {}
};

struct SphereCollider : ColliderBase {
    float   Radius = 0.5f;
    Vector3 Offset = {0.0f, 0.0f, 0.0f};

    SphereCollider() = default;

    SphereCollider(const SphereCollider&) = default;

    SphereCollider(float radius) : Radius(radius) {}
};

struct CapsuleCollider : ColliderBase {
    float   Radius = 0.5f;
    float   Height = 1.0f;
    Vector3 Offset = {0.0f, 0.0f, 0.0f};

    CapsuleCollider() = default;

    CapsuleCollider(const CapsuleCollider&) = default;

    CapsuleCollider(float radius, float height) : Radius(radius), Height(height) {}
};
} // namespace se