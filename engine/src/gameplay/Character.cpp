#include "engine/gameplay/Character.h"
#include "engine/gameplay/Controller.h"
#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/physics/PhysicsSystem.h"
#include "engine/physics/Collider.h"
#include "engine/Log.h"

#include <glm.hpp>
#include <cmath>

namespace se {

namespace {
float NormalizeAngle(float angle) {
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}
}  // namespace

void Character::Awake() {
    SetupPhysics();
}

void Character::Start() {
    rigidbody_ = GetEntity().FindComponent<RigidbodyComponent>();
    physicsSystem_ = GetScene()->GetPhysicsSystem();
    
    if (!rigidbody_) {
        SE_LOG_ERROR("[Character] No RigidbodyComponent found on entity {}", GetEntityID());
    }
}

void Character::Update(float dt) {
    Pawn::Update(dt);
    
    // Consume movement input from controller and convert to movement
    Vector3 input = ConsumeMovementInput();
    if (glm::length(input) > 0.01f) {
        // Convert controller input to world-space movement
        // If oriented to camera, use control rotation
        if (movementConfig_.orientToMovement && controller_) {
            float yaw = glm::radians(controlRotation_.y);
            Vector3 forward{-std::sin(yaw), 0.0f, -std::cos(yaw)};
            Vector3 right{std::cos(yaw), 0.0f, -std::sin(yaw)};
            
            Vector3 worldDirection = forward * input.z + right * input.x;
            if (glm::length(worldDirection) > 0.01f) {
                desiredMoveDirection_ = glm::normalize(worldDirection);
                wantsToMove_ = true;
                
                // Face movement direction - use atan2(x, z) to match original behavior
                targetYaw_ = glm::degrees(std::atan2(desiredMoveDirection_.x, desiredMoveDirection_.z));
                hasTargetRotation_ = true;
            }
        } else {
            desiredMoveDirection_ = glm::normalize(input);
            wantsToMove_ = true;
        }
    } else {
        wantsToMove_ = false;
    }
}

void Character::FixedUpdate(float dt) {
    UpdateGroundedState();
    ApplyJump();
    ApplyRotation(dt);
    ApplyMovement(dt);
    ApplyDrag(dt);
}

void Character::SetupPhysics() {
    auto* scene = GetScene();
    if (!scene || !scene->HasPhysics()) {
        SE_LOG_ERROR("[Character] Cannot setup physics: scene or physics system missing");
        return;
    }

    // Add capsule collider
    CapsuleCollider collider;
    collider.Height = physicsConfig_.height;
    collider.Radius = physicsConfig_.radius;
    
    Entity entity = GetEntity();
    entity.AddComponent<CapsuleCollider>(collider);

    // Add rigidbody with frozen rotation (rotation controlled via code)
    RigidbodyData rigidData{
        .mass            = physicsConfig_.mass,
        .gravityScale    = movementConfig_.gravityScale,
        .material        = PhysicsMaterial(2.0f, 1.0f),
        .freezeRotationX = true,
        .freezeRotationY = true,
        .freezeRotationZ = true
    };

    entity.AddComponent<RigidbodyComponent>(rigidData);
    SE_LOG_DEBUG("[Character] Physics setup complete for entity {}", GetEntityID());
}

void Character::Move(const Vector3& direction) {
    if (glm::length(direction) > 0.01f) {
        desiredMoveDirection_ = glm::normalize(direction);
        wantsToMove_ = true;
    }
}

void Character::Jump() {
    wantsToJump_ = true;
}

void Character::StopJumping() {
    wantsToJump_ = false;
}

bool Character::IsMoving() const {
    Vector3 vel = GetVelocity();
    return std::abs(vel.x) > 0.1f || std::abs(vel.z) > 0.1f;
}

Vector3 Character::GetVelocity() const {
    if (!rigidbody_) return Vector3{0.0f};
    btVector3 btVel = rigidbody_->GetLinearVelocity();
    return Vector3{btVel.x(), btVel.y(), btVel.z()};
}

void Character::SetVelocity(const Vector3& velocity) {
    if (!rigidbody_) return;
    rigidbody_->SetLinearVelocity(btVector3{velocity.x, velocity.y, velocity.z});
}

void Character::UpdateGroundedState() {
    if (!physicsSystem_ || !rigidbody_) {
        isGrounded_ = false;
        return;
    }

    auto& transform = GetComponent<TransformComponent>();
    Vector3 start = transform.Position;

    float rayLength = physicsConfig_.height * 0.5f + physicsConfig_.radius + 0.15f;
    Vector3 end = start - Vector3{0.0f, rayLength, 0.0f};

    Vector3 hitPoint, hitNormal;
    btRigidBody* ownBody = rigidbody_->GetRigidbody();

    isGrounded_ = physicsSystem_->Raycast(start, end, hitPoint, hitNormal, ownBody);
}

void Character::ApplyMovement(float dt) {
    if (!rigidbody_ || !wantsToMove_) return;

    Vector3 currentVel = GetVelocity();
    Vector3 horizontalVel{currentVel.x, 0.0f, currentVel.z};

    float maxSpeed = movementConfig_.maxWalkSpeed;
    Vector3 targetVel = desiredMoveDirection_ * maxSpeed;
    
    float accel = isGrounded_ ? movementConfig_.acceleration : 
                                movementConfig_.acceleration * movementConfig_.airControl;
    
    Vector3 velocityChange = (targetVel - horizontalVel) * accel * dt;
    Vector3 newHorizontalVel = horizontalVel + velocityChange;

    float speed = glm::length(newHorizontalVel);
    if (speed > maxSpeed) {
        newHorizontalVel = glm::normalize(newHorizontalVel) * maxSpeed;
    }

    SetVelocity(Vector3{newHorizontalVel.x, currentVel.y, newHorizontalVel.z});
}

void Character::ApplyRotation(float dt) {
    if (!rigidbody_ || !hasTargetRotation_) return;

    auto& transform = GetComponent<TransformComponent>();
    float currentYaw = NormalizeAngle(transform.Rotation.y);
    float diff = NormalizeAngle(targetYaw_ - currentYaw);

    // Use rotationSpeed as interpolation factor (like old code)
    float t = glm::clamp(movementConfig_.rotationSpeed * dt, 0.0f, 1.0f);
    float newYaw = NormalizeAngle(currentYaw + diff * t);

    rigidbody_->SetRotation({0.0f, newYaw, 0.0f});
    hasTargetRotation_ = false;
}

void Character::ApplyJump() {
    if (!rigidbody_ || !isGrounded_ || !wantsToJump_) {
        wantsToJump_ = false;
        return;
    }

    Vector3 velocity = GetVelocity();
    velocity.y = movementConfig_.jumpForce;
    SetVelocity(velocity);
    wantsToJump_ = false;
    
    SE_LOG_DEBUG("[Character] Jump applied with force {}", movementConfig_.jumpForce);
}

void Character::ApplyDrag(float dt) {
    if (!rigidbody_) return;
    if (wantsToMove_) return;

    Vector3 vel = GetVelocity();
    float drag = isGrounded_ ? movementConfig_.groundFriction : (1.0f - movementConfig_.airControl);

    vel.x *= (1.0f - drag * dt);
    vel.z *= (1.0f - drag * dt);
    SetVelocity(vel);
}

}  // namespace se
