#include "apps/third_person_game/src/Character.h"

#include "engine/ecs/Scene.h"

namespace FirstGame {

void Character::Awake() {
    RigidbodyData data{.mass           = 0.0f,
                       .gravityScale   = 1.0f,
                       .material       = PhysicsMaterial(0.8f, 0.8f),
                       .freezeRotationX = true,
                       .freezeRotationZ = true};
    auto rb = GetEntity().AddComponent<RigidbodyComponent>(data);
    rb.SetAngularFactor({0.0f, 1.0f, 0.0f});

    CapsuleCollider collider;
    collider.Height = specs_.character_height_;
    collider.Radius = specs_.character_radius_;
    GetEntity().AddComponent<CapsuleCollider>(collider);
    
    SE_LOG_INFO("Character::Awake() - Physics components added");
}

void Character::Start() {
    SE_LOG_INFO("Character::Start() - Character ready for entity {}", GetEntity().GetID());
}

void Character::Update(float dt) {
    // Character-specific update logic here
}

} // namespace FirstGame