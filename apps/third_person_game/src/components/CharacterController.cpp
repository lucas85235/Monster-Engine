#include "CharacterController.h"

#include "CameraController.h"
#include "Character.h"
#include "engine/Application.h"
#include <mmath/MathUtils.h>
#include "engine/ecs/SimpleComponents.h"

namespace FirstGame {


void CharacterController::Awake() {
    BindInputs();
}

void CharacterController::Start() {
    CacheComponents();
    SetMouseCaptured(true);
}

void CharacterController::Update(float dt) {
    ProcessInput(dt);
}

void CharacterController::OnDestroy() {
    auto& input = InputManager::Get();
    input.PopContext(inputMapName_);
    input.RemoveActionMap(inputMapName_);
}



void CharacterController::BindInputs() {
    auto& input = InputManager::Get();
    input.CreateActionMap(inputMapName_);
    input.PushContext(inputMapName_);

    // Mouse toggle
    input.BindAction(inputMapName_, cameraBindings_.toggleMouse, Key::Tab);

    // Camera rotation
    input.BindAxis(inputMapName_, cameraBindings_.cameraRotateX, Key::MouseX, 1.0f);
    input.BindAxis(inputMapName_, cameraBindings_.cameraRotateY, Key::MouseY, -1.0f);

    // Movement
    input.BindAxis(inputMapName_, movementConfig_.moveForward, Key::W, 1.0f);
    input.BindAxis(inputMapName_, movementConfig_.moveForward, Key::S, -1.0f);
    input.BindAxis(inputMapName_, movementConfig_.moveRight, Key::D, 1.0f);
    input.BindAxis(inputMapName_, movementConfig_.moveRight, Key::A, -1.0f);

    // Actions
    input.BindAction(inputMapName_, movementConfig_.jump, Key::Space);
}

void CharacterController::CacheComponents() {
    Entity entity = GetEntity();

    // Get Character component (lifecycle)
    character_ = entity.FindComponent<Character>();
    if (!character_) {
        SE_LOG_WARN("CharacterController: No Character component found!");
    }

    // Get CameraController (lifecycle)
    cameraController_ = entity.FindComponent<CameraController>();
}



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
    InputManager::Get().SetCursorMode(captured ? CursorMode::Locked : CursorMode::Normal);

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
    bool isMoving = glm::length(moveDir) > 0.01f;
    
    if (isMoving) {
        float targetYaw = luma::YawFromDirection(moveDir.x, moveDir.z);
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
