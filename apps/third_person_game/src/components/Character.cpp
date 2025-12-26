#include "Character.h"

#include "apps/MathUtils.h"
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
    // Freeze ALL physical rotations - rotation is controlled via code in RotateTowards()
    RigidbodyData rigidData{
        .mass            = physicsConfig_.mass,
        .gravityScale    = 1.0f,
        .material        = PhysicsMaterial(0.5f, 0.0f),
        .freezeRotationX = true,
        .freezeRotationY = true,
        .freezeRotationZ = true
    };

    GetEntity().AddComponent<RigidbodyComponent>(rigidData);
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

void Character::RotateTowards(float targetYaw) {
    if (!rigidbody_) return;

    auto& transform  = GetComponent<TransformComponent>();
    float currentYaw = Math::NormalizeAngle(transform.Rotation.y);
    float diff       = Math::NormalizeAngle(targetYaw - currentYaw);

    float t      = glm::clamp(movementConfig_.rotationSpeed * Time::DeltaTime(), 0.0f, 1.0f);
    float newYaw = Math::NormalizeAngle(currentYaw + diff * t);

    rigidbody_->SetRotation({0.0f, newYaw, 0.0f});
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