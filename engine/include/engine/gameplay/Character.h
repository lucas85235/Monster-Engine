#pragma once

#include "engine/gameplay/Pawn.h"
#include "engine/physics/RigidbodyComponent.h"

namespace se {

class PhysicsSystem;

struct CharacterMovementConfig {
    float maxWalkSpeed   = 5.0f;
    float maxRunSpeed    = 8.0f;
    float acceleration   = 30.0f;
    float deceleration   = 20.0f;
    float groundFriction = 9.0f;   // groundDrag from old code
    float airControl     = 0.02f;  // 1 - airDrag (0.98)
    float jumpForce      = 5.0f;   // match old code
    float gravityScale   = 1.0f;
    float rotationSpeed  = 15.0f;  // interpolation factor per second (match old code)
    bool  orientToMovement = true;
};

struct CharacterPhysicsConfig {
    float height = 1.1f;   // match old code
    float radius = 0.2f;   // match old code
    float mass   = 70.0f;  // match old code
};

class Character : public Pawn {
   public:
    Character()           = default;
    ~Character() override = default;

    void Awake() override;
    void Start() override;
    void Update(float dt) override;
    void FixedUpdate(float dt) override;

    // Movement API (high-level)
    void Move(const Vector3& direction);
    void Jump();
    void StopJumping();

    // State queries
    bool    IsMoving() const;
    bool    IsGrounded() const { return isGrounded_; }
    bool    IsFalling() const { return !isGrounded_; }
    Vector3 GetVelocity() const;
    void    SetVelocity(const Vector3& velocity);
    
    // AI-controlled rotation (bypasses camera-relative transformation)
    void    SetTargetRotation(float yaw) { targetYaw_ = yaw; hasTargetRotation_ = true; }

    // Configuration access
    CharacterMovementConfig& GetMovementConfig() { return movementConfig_; }
    CharacterPhysicsConfig&  GetPhysicsConfig() { return physicsConfig_; }
    const CharacterMovementConfig& GetMovementConfig() const { return movementConfig_; }
    const CharacterPhysicsConfig&  GetPhysicsConfig() const { return physicsConfig_; }

    // Rigidbody access
    RigidbodyComponent* GetRigidbody() const { return rigidbody_; }

   protected:
    void SetupPhysics();
    void UpdateGroundedState();
    void ApplyMovement(float dt);
    void ApplyRotation(float dt);
    void ApplyJump();
    void ApplyDrag(float dt);

    // Physics
    RigidbodyComponent* rigidbody_     = nullptr;
    PhysicsSystem*      physicsSystem_ = nullptr;

    // Configuration
    CharacterMovementConfig movementConfig_;
    CharacterPhysicsConfig  physicsConfig_;

    // Movement state
    Vector3 desiredMoveDirection_{0.0f};
    bool    wantsToMove_       = false;
    bool    wantsToJump_       = false;
    float   targetYaw_         = 0.0f;
    bool    hasTargetRotation_ = false;

    // Grounded state
    bool isGrounded_ = false;
};

}  // namespace se
