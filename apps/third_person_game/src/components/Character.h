#pragma once
/**
 * Character.h - Core character data and physics component.
 *
 * This component manages:
 * - Character specifications (movement params, dimensions)
 * - Physics setup (rigidbody, collider)
 * - Character state (grounded, jumping, etc.)
 *
 * Requires: TransformComponent (auto-added by Entity)
 * Adds: CapsuleCollider, RigidbodyComponent
 */

#include "engine/physics/Collider.h"
#include "engine/physics/RigidbodyComponent.h"

namespace FirstGame {
using namespace se;

// ============================================================================
// Character Configuration
// ============================================================================

struct CharacterMovementConfig {
    float acceleration     = 3.4f;
    float maxMovementSpeed = 5.0f;
    float maxRotationSpeed = 180.0f;
    float jumpForce        = 5.0f;
    float groundDrag       = 0.9f;
    float airDrag          = 0.98f;
};

struct CharacterPhysicsConfig {
    float height = 1.8f;
    float radius = 0.3f;
    float mass   = 70.0f;
};

// ============================================================================
// Character Component
// ============================================================================

class Character : public Component {
public:
    Character() = default;
    ~Character() override = default;

    // Lifecycle
    void Awake() override;
    void Start() override;
    void Update(float dt) override;

    // === Movement API ===
    void Move(const Vector3& direction);
    void Jump();
    void StopMovement();

    // === State Queries ===
    bool IsGrounded() const { return isGrounded_; }
    bool IsMoving() const;

    // === Rigidbody Access ===
    RigidbodyComponent* GetRigidbody() const { return rigidbody_; }
    Vector3             GetVelocity() const;
    void                SetVelocity(const Vector3& velocity);

    // === Configuration ===
    CharacterMovementConfig& GetMovementConfig() { return movementConfig_; }
    CharacterPhysicsConfig&  GetPhysicsConfig() { return physicsConfig_; }

private:
    void SetupPhysics();
    void UpdateGroundedState();

    // Component references
    RigidbodyComponent* rigidbody_ = nullptr;

    // Configuration
    CharacterMovementConfig movementConfig_;
    CharacterPhysicsConfig  physicsConfig_;

    // State
    bool isGrounded_ = false;
};

} // namespace FirstGame
