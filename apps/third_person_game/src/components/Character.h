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
    float acceleration     = 0.5f;
    float maxMovementSpeed = 5.0f;
    float rotationSpeed    = 30.0f;
    float jumpForce        = 5.0f;
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
};
} // namespace FirstGame
