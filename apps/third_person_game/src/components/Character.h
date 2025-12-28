#pragma once
/**
 * Character - Core physics and movement logic.
 */

#include "engine/physics/Collider.h"
#include "engine/physics/PhysicsSystem.h"
#include "engine/physics/RigidbodyComponent.h"

namespace FirstGame {
using namespace se;



struct CharacterMovementConfig {
    // Values are per-second (multiply by dt when used)
    float acceleration     = 30.0f;   //  ~30/s for similar feel at 60fps
    float maxMovementSpeed = 5.0f;
    float rotationSpeed    = 15.0f;   // Degrees per second - increased for snappier rotation
    float jumpForce        = 5.0f;    // Impulse - not affected by dt
    float groundDrag       = 9.0f;
    float airDrag          = 0.98f;
};

struct CharacterPhysicsConfig {
    float height = 1.1f;
    float radius = 0.2f;
    float mass   = 70.0f;
};



class Character : public Component {
public:
    Character() = default;

    ~Character() override = default;

    // Lifecycle
    void Awake() override;

    void Start() override;

    void Update(float dt) override;
    
    void FixedUpdate(float dt) override;

    // Movement API
    void Move(const Vector3& direction);

    void RotateTowards(float targetYaw);

    void Jump();

    void StopMovement();

    // State queries
    bool IsGrounded() const { return isGrounded_; }

    bool IsMoving() const;

    // Rigidbody access
    RigidbodyComponent* GetRigidbody() const { return rigidbody_; }

    Vector3 GetVelocity() const;

    void SetVelocity(const Vector3& velocity);

    // Configuration queries
    CharacterMovementConfig& GetMovementConfig() { return movementConfig_; }
    CharacterPhysicsConfig&  GetPhysicsConfig() { return physicsConfig_; }

private:
    void SetupPhysics();

    void UpdateGroundedState();

    void ApplyRotation(float dt);
    
    void ApplyJump();
    
    void ApplyMovement(float dt);

    void ApplyDrag(float dt);

    // Component references
    RigidbodyComponent* rigidbody_     = nullptr;
    PhysicsSystem*      physicsSystem_ = nullptr;

    // Configuration
    CharacterMovementConfig movementConfig_;
    CharacterPhysicsConfig  physicsConfig_;

    // State
    bool    isGrounded_           = false;
    Vector3 desiredMoveDirection_ = Vector3(0.0f);
    bool    wantsToMove_          = false;
    float   targetYaw_            = 0.0f;
    bool    hasTargetRotation_    = false;
    bool    wantsToJump_          = false;
};
} // namespace FirstGame
