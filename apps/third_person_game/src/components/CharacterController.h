#pragma once

#include "Engine.h"
#include "engine/Camera.h"
#include "engine/input/InputManager.h"

namespace se {
class RigidbodyComponent;
struct SpringArmComponent;
struct TransformComponent;
} // namespace se

namespace FirstGame {
using namespace se;

/**
 * CharacterController - Component that handles player input and camera control.
 *
 * Responsibilities:
 * - Input handling (mouse capture, movement input, camera rotation)
 * - Camera positioning and spring arm logic
 * - Character movement via rigidbody physics
 *
 * This component requires the entity to have:
 * - TransformComponent (added automatically)
 * - SpringArmComponent (added in Awake)
 * - RigidbodyComponent (expected from Character component)
 */
class CharacterController : public Component {
public:
    CharacterController() = default;
    ~CharacterController() override = default;

    void Awake() override;
    void Start() override;
    void Update(float dt) override;

    Camera* GetCamera() const { return camera_; }

    // Configuration
    float GetCameraSensitivity() const { return cameraSensitivity_; }
    void  SetCameraSensitivity(float sensitivity) { cameraSensitivity_ = sensitivity; }

    float GetMoveSpeed() const { return moveSpeed_; }
    void  SetMoveSpeed(float speed) { moveSpeed_ = speed; }

private:
    // === Initialization ===
    void InitializeCamera();
    void InitializeSpringArm();
    void BindInputActions();

    // === Update Logic ===
    void UpdateInputState();
    void UpdateMovement(float dt);
    void UpdateCameraPosition(float dt);

    // === Input Helpers ===
    void HandleMouseCapture();
    glm::vec2 GetCameraRotationInput() const;
    glm::vec2 GetMovementInput() const;

    // === Camera Helpers ===
    void UpdateSpringArmRotation(float mouseX, float mouseY);
    void CalculateCameraDirection(float yawRad, float pitchRad, glm::vec3& outDirection) const;
    float CalculateArmLengthWithCollision(const glm::vec3& targetPos, const glm::vec3& direction);
    void ApplyCameraTransform(const glm::vec3& position, float yaw, float pitch);

    // === Movement Helpers ===
    void ApplyMovementVelocity(const glm::vec2& moveInput, const glm::vec3& camForward, const glm::vec3& camRight);

    // === Component References (cached in Start) ===
    RigidbodyComponent*   rigidbody_   = nullptr;
    SpringArmComponent*   springArm_   = nullptr;
    TransformComponent*   transform_   = nullptr;

    // === State ===
    Camera* camera_        = nullptr;
    bool    mouseCaptured_ = false;

    // === Configuration ===
    float cameraSensitivity_ = 0.1f;
    float moveSpeed_         = 5.0f;
    float cameraLerpSpeedIn_ = 15.0f;
    float cameraLerpSpeedOut_ = 5.0f;
    float minArmLength_      = 0.5f;
};
} // namespace FirstGame