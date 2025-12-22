#include "apps/third_person_game/src/Character.h"

#include "engine/ecs/Scene.h"

namespace FirstGame {
Character::Character(Entity entity) : entity_(entity) {
    RigidbodyData data{.mass            = 0.0f,
                       .gravityScale    = 1.0f,
                       .material        = PhysicsMaterial(0.8f, 0.8f),
                       .freezeRotationX = true,
                       .freezeRotationZ = true};
    auto rb = entity_.AddComponent<RigidbodyComponent>(data, entity_);
    rb.SetAngularFactor({0.0f, 1.0f, 0.0f});

    CapsuleCollider collider;
    collider.Height = specs_.character_height_;
    collider.Radius = specs_.character_radius_;
    entity_.AddComponent<CapsuleCollider>(collider);
}
}  // namespace FirstGame