#include "Character.h"

#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"

namespace FirstGame {

// ============================================================================
// Lifecycle
// ============================================================================

void Character::Awake() {
    SetupPhysics();
    SE_LOG_INFO("Character::Awake() - Physics initialized");
}

void Character::Start() {
    // Cache rigidbody reference (it's a lifecycle component)
    rigidbody_ = GetEntity().GetScript<RigidbodyComponent>();

    if (rigidbody_) {
        SE_LOG_INFO("Character::Start() - Rigidbody cached (body: {})",
                    (void*)rigidbody_->GetRigidbody());
    } else {
        SE_LOG_ERROR("Character::Start() - Failed to get RigidbodyComponent!");
    }
}

void Character::Update(float dt) {
    UpdateGroundedState();
}

// ============================================================================
// Physics Setup
// ============================================================================

void Character::SetupPhysics() {
    // IMPORTANT: Add collider BEFORE RigidbodyComponent!
    // RigidbodyComponent::Awake() reads the collider shape.
    CapsuleCollider collider;
    collider.Height = physicsConfig_.height;
    collider.Radius = physicsConfig_.radius;
    GetEntity().AddComponent<CapsuleCollider>(collider);

    // Now add the rigidbody - it will detect the CapsuleCollider
    RigidbodyData rigidData{
        .mass            = physicsConfig_.mass,
        .gravityScale    = 1.0f,
        .material        = PhysicsMaterial(0.5f, 0.3f),
        .freezeRotationX = true,
        .freezeRotationY = false,
        .freezeRotationZ = true
    };

    auto& rb = GetEntity().AddComponent<RigidbodyComponent>();
    rb.SetData(rigidData);
    rb.SetAngularFactor({0.0f, 1.0f, 0.0f});
}

// ============================================================================
// Movement API
// ============================================================================

void Character::Move(const Vector3& direction) {
    if (!rigidbody_) return;

    Vector3 velocity = direction * movementConfig_.maxMovementSpeed;
    velocity.y       = GetVelocity().y; // Preserve vertical velocity

    SetVelocity(velocity);
}

void Character::Jump() {
    if (!rigidbody_ || !isGrounded_) return;

    Vector3 velocity = GetVelocity();
    velocity.y       = movementConfig_.jumpForce;
    SetVelocity(velocity);
}

void Character::StopMovement() {
    if (!rigidbody_) return;

    Vector3 velocity = GetVelocity();
    velocity.x       = 0.0f;
    velocity.z       = 0.0f;
    SetVelocity(velocity);
}

// ============================================================================
// State Queries
// ============================================================================

bool Character::IsMoving() const {
    Vector3 vel = GetVelocity();
    return std::abs(vel.x) > 0.1f || std::abs(vel.z) > 0.1f;
}

Vector3 Character::GetVelocity() const {
    if (!rigidbody_) return Vector3(0.0f);
    return ToGlm(rigidbody_->GetLinearVelocity());
}

void Character::SetVelocity(const Vector3& velocity) {
    if (!rigidbody_) return;
    rigidbody_->SetLinearVelocity(ToBt(velocity));
}

// ============================================================================
// Internal
// ============================================================================

void Character::UpdateGroundedState() {
    // TODO: Implement proper ground check via raycast
    // For now, consider grounded if vertical velocity is near zero
    isGrounded_ = std::abs(GetVelocity().y) < 0.1f;
}

} // namespace FirstGame