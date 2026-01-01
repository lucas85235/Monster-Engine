#include "Character.h"

#include <glm.hpp>
#include "apps/MathUtils.h"
#include "engine/Log.h"
#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/physics/PhysicsDebugDraw.h"

namespace FirstGame {


void Character::Awake() {
    SetupPhysics();
}

void Character::Start() {
    rigidbody_ = GetEntity().FindComponent<RigidbodyComponent>();
    physicsSystem_ = GetEntity().GetScene()->GetPhysicsSystem();
}

void Character::Update(float dt) {
    // Reset movement state at start of each frame
    // This ensures all FixedUpdates in a frame see consistent state
    wantsToMove_ = false;
}

void Character::FixedUpdate(float dt) {
    // Physics runs at fixed 60Hz - this prevents framerate-dependent behavior
    UpdateGroundedState();
    ApplyJump();          // Process jump request (must be before movement)
    ApplyRotation(dt);    // Process rotation in fixed timestep
    ApplyMovement(dt);
    ApplyDrag(dt);
}



void Character::SetupPhysics() {
    // Rigidbody setup requires collider to be present.
    CapsuleCollider collider;
    collider.Height = physicsConfig_.height;
    collider.Radius = physicsConfig_.radius;
    GetEntity().GetScene()->GetPhysicsSystem()->GetDebugDrawer()->SetMode(se::PhysicsDebugDraw::DebugDrawMode::None);
    GetEntity().AddComponent<CapsuleCollider>(collider);

    // Now add the rigidbody - it will detect the CapsuleCollider
    // Freeze ALL physical rotations - rotation is controlled via code in RotateTowards()
    RigidbodyData rigidData{
        .mass            = physicsConfig_.mass,
        .gravityScale    = 1.0f,
        .material        = PhysicsMaterial(2.0f, 1.0f),
        .freezeRotationX = true,
        .freezeRotationY = true,
        .freezeRotationZ = true
    };

    GetEntity().AddComponent<RigidbodyComponent>(rigidData);
}



void Character::Move(const Vector3& direction) {
    if (!rigidbody_) return;

    if (glm::length(direction) > 0.01f) {
        desiredMoveDirection_ = glm::normalize(direction);
        wantsToMove_ = true;
    }
}

void Character::RotateTowards(float targetYaw) {
    // Store target for processing in FixedUpdate
    targetYaw_ = targetYaw;
    hasTargetRotation_ = true;
}

void Character::ApplyRotation(float dt) {
    if (!rigidbody_ || !hasTargetRotation_) return;

    auto& transform  = GetComponent<TransformComponent>();
    float currentYaw = Math::NormalizeAngle(transform.Rotation.y);
    float diff       = Math::NormalizeAngle(targetYaw_ - currentYaw);

    // Use fixed dt for framerate-independent rotation
    float t      = glm::clamp(movementConfig_.rotationSpeed * dt, 0.0f, 1.0f);
    float newYaw = Math::NormalizeAngle(currentYaw + diff * t);

    rigidbody_->SetRotation({0.0f, newYaw, 0.0f});
    hasTargetRotation_ = false;
}

void Character::Jump() {
    // Store jump request to be processed in FixedUpdate
    wantsToJump_ = true;
}

void Character::ApplyJump() {
    if (!rigidbody_ || !isGrounded_ || !wantsToJump_) {
        wantsToJump_ = false;
        return;
    }

    Vector3 velocity = GetVelocity();
    velocity.y       = movementConfig_.jumpForce;
    SetVelocity(velocity);
    wantsToJump_ = false;
}

void Character::StopMovement() {
    if (!rigidbody_) return;

    Vector3 velocity = GetVelocity();
    velocity.x       = 0.0f;
    velocity.z       = 0.0f;
    SetVelocity(velocity);
}



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



void Character::UpdateGroundedState() {
    if (!physicsSystem_ || !rigidbody_) {
        isGrounded_ = false;
        return;
    }

    auto& transform = GetComponent<TransformComponent>();
    Vector3 start = transform.Position;

    // Ray starts at capsule center, goes down past the bottom
    float rayLength = physicsConfig_.height * 0.5f + physicsConfig_.radius + 0.15f;
    Vector3 end = start - Vector3(0.0f, rayLength, 0.0f);

    Vector3 hitPoint, hitNormal;
    btRigidBody* ownBody = rigidbody_->GetRigidbody();

    isGrounded_ = physicsSystem_->Raycast(start, end, hitPoint, hitNormal, ownBody);
}

void Character::ApplyMovement(float dt) {
    if (!rigidbody_ || !wantsToMove_) return;

    Vector3 currentVel = GetVelocity();
    Vector3 horizontalVel(currentVel.x, 0.0f, currentVel.z);

    // Apply acceleration towards desired direction (frame-rate independent)
    Vector3 targetVel = desiredMoveDirection_ * movementConfig_.maxMovementSpeed;
    Vector3 velocityChange = (targetVel - horizontalVel) * movementConfig_.acceleration * dt;

    Vector3 newHorizontalVel = horizontalVel + velocityChange;

    // Clamp to max speed
    float speed = glm::length(newHorizontalVel);
    if (speed > movementConfig_.maxMovementSpeed) {
        newHorizontalVel = glm::normalize(newHorizontalVel) * movementConfig_.maxMovementSpeed;
    }
    
    // Debug log every ~1 second (60 fixed updates)
    static int debugCounter = 0;
    if (++debugCounter >= 60) {
        debugCounter = 0;
        SE_LOG_INFO("[MOVE] dt={:.4f}, accel={:.1f}, vel=({:.2f},{:.2f},{:.2f}), speed={:.2f}, grounded={}",
                    dt, movementConfig_.acceleration, 
                    newHorizontalVel.x, currentVel.y, newHorizontalVel.z,
                    speed, isGrounded_);
    }

    SetVelocity(Vector3(newHorizontalVel.x, currentVel.y, newHorizontalVel.z));
    // Note: wantsToMove_ is NOT reset here - it stays true for all FixedUpdates in a frame
    // and gets reset at the start of the next Update()
}

void Character::ApplyDrag(float dt) {
    if (!rigidbody_) return;

    // Only apply drag when not actively moving
    if (wantsToMove_) return;

    Vector3 vel = GetVelocity();
    float drag = isGrounded_ ? movementConfig_.groundDrag : movementConfig_.airDrag;

    // Apply drag to horizontal velocity
    vel.x *= (1.0f - drag * dt);
    vel.z *= (1.0f - drag * dt);
    SetVelocity(vel);
}
} // namespace FirstGame