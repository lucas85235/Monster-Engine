#include "CharacterController.h"

#include "CameraController.h"
#include "Character.h"
#include "engine/core/Application.h"
#include "apps/MathUtils.h"
#include "engine/ecs/SimpleComponents.h"

namespace FirstGame {
// ============================================================================
// Lifecycle
// ============================================================================

void CharacterController::Awake() {
    BindInputs();
    SE_LOG_INFO("CharacterController::Awake() - Input bindings set");
}

void CharacterController::Start() {
    CacheComponents();
    SE_LOG_INFO("CharacterController::Start() - Ready");
}

void CharacterController::Update(float dt) {
    ProcessInput(dt);
}

// ============================================================================
// Initialization
// ============================================================================

void CharacterController::BindInputs() {
    auto& input = InputManager::Get();

    // Mouse toggle
    input.BindAction(cameraBindings_.toggleMouse, Key::Tab);

    // Camera rotation
    input.BindAxis(cameraBindings_.cameraRotateX, Key::MouseX, 1.0f);
    input.BindAxis(cameraBindings_.cameraRotateY, Key::MouseY, -1.0f);

    // Movement
    input.BindAxis(movementConfig_.moveForward, Key::W, 1.0f);
    input.BindAxis(movementConfig_.moveForward, Key::S, -1.0f);
    input.BindAxis(movementConfig_.moveRight, Key::D, 1.0f);
    input.BindAxis(movementConfig_.moveRight, Key::A, -1.0f);

    // Actions
    input.BindAction(movementConfig_.jump, Key::Space);
}

void CharacterController::CacheComponents() {
    Entity entity = GetEntity();

    // Get Character component (lifecycle)
    character_ = entity.GetScript<Character>();
    if (!character_) {
        SE_LOG_WARN("CharacterController: No Character component found!");
    }

    // Get CameraController (lifecycle)
    cameraController_ = entity.GetScript<CameraController>();
    if (!cameraController_) {
        SE_LOG_WARN("CharacterController: No CameraController component found!");
    }
}

// ============================================================================
// Input Processing
// ============================================================================

void CharacterController::ProcessInput(float /* unused - using Time::DeltaTime() */) {
    HandleMouseToggle();

    if (mouseCaptured_) {
        ProcessMovementInput();
        ProcessCameraInput();
    }
}

void CharacterController::HandleMouseToggle() {
    auto& input = InputManager::Get();

    if (input.IsActionJustPressed(cameraBindings_.toggleMouse)) {
        SetMouseCaptured(!mouseCaptured_);
    }
}

void CharacterController::SetMouseCaptured(bool captured) {
    mouseCaptured_ = captured;

    auto* window = Application::Get().GetWindow().GetNativeWindow();
    glfwSetInputMode(window, GLFW_CURSOR,
                     captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

    SE_LOG_INFO("Mouse capture: {}", captured ? "enabled" : "disabled");
}

void CharacterController::ProcessMovementInput() {
    if (!character_ || !cameraController_) return;

    auto& input = InputManager::Get();

    float moveX = input.GetAxis(movementConfig_.moveRight);
    float moveZ = input.GetAxis(movementConfig_.moveForward);

    // Get camera-relative directions
    Vector3 forward = cameraController_->GetForwardDirection();
    Vector3 right   = cameraController_->GetRightDirection();

    Vector3 moveDir = forward * moveZ + right * moveX;

    // Rotate towards movement direction
    if (glm::length(moveDir) > 0.01f) {
        float targetYaw = Math::CalculateYawFromDirection(moveDir.x, moveDir.z);
        character_->RotateTowards(targetYaw);

        moveDir = glm::normalize(moveDir);
        character_->Move(moveDir);
    }

    // Jump
    if (input.IsActionJustPressed(movementConfig_.jump)) {
        character_->Jump();
    }
}

void CharacterController::ProcessCameraInput() {
    if (!cameraController_) return;

    auto& input = InputManager::Get();

    float deltaX = input.GetAxis(cameraBindings_.cameraRotateX);
    float deltaY = input.GetAxis(cameraBindings_.cameraRotateY);

    cameraController_->RotateCamera(deltaX, deltaY);
}

} // namespace FirstGame