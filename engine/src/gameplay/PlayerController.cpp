#include "engine/gameplay/PlayerController.h"
#include "engine/gameplay/Pawn.h"
#include "engine/gameplay/Character.h"
#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/physics/PhysicsSystem.h"
#include "engine/physics/RigidbodyComponent.h"
#include "engine/input/InputManager.h"
#include "engine/Application.h"
#include "engine/Log.h"

#include <cmath>

namespace se {

void PlayerController::Awake() {
    BindInputs();
    
    currentArmLength_ = cameraConfig_.targetArmLength;
    controlRotation_.x = cameraConfig_.initialPitch;
}

void PlayerController::Start() {
    Controller::Start();
    
    // Create camera if not set
    if (!camera_) {
        camera_ = new Camera(Vector3{0.0f, 5.0f, 10.0f});
        SE_LOG_DEBUG("[PlayerController] Created default camera");
    }
    
    // Set as active camera
    if (camera_ && GetScene()) {
        GetScene()->SetActiveCamera(camera_);
    }
    
    // Cache physics system
    if (GetScene() && GetScene()->HasPhysics()) {
        physicsSystem_ = GetScene()->GetPhysicsSystem();
    }
}

void PlayerController::Update(float dt) {
    if (!inputEnabled_) return;
    
    ProcessInput(dt);
}

void PlayerController::LateUpdate(float dt) {
    UpdateCamera(dt);
}

void PlayerController::OnPossess(Pawn* pawn) {
    SE_LOG_INFO("[PlayerController] Possessed pawn {}", pawn->GetEntityID());
}

void PlayerController::OnUnpossess() {
    SE_LOG_INFO("[PlayerController] Unpossessed pawn");
}

void PlayerController::BindInputs() {
    auto& input = InputManager::Get();

    // Mouse toggle
    input.BindAction(inputConfig_.toggleMouse, Key::Tab);

    // Camera rotation
    input.BindAxis(inputConfig_.lookX, Key::MouseX, 1.0f);
    input.BindAxis(inputConfig_.lookY, Key::MouseY, -1.0f);

    // Movement
    input.BindAxis(inputConfig_.moveForward, Key::W, 1.0f);
    input.BindAxis(inputConfig_.moveForward, Key::S, -1.0f);
    input.BindAxis(inputConfig_.moveRight, Key::D, 1.0f);
    input.BindAxis(inputConfig_.moveRight, Key::A, -1.0f);

    // Actions
    input.BindAction(inputConfig_.jump, Key::Space);
}

void PlayerController::ProcessInput(float dt) {
    auto& input = InputManager::Get();
    
    // Handle mouse toggle
    if (input.IsActionJustPressed(inputConfig_.toggleMouse)) {
        ToggleMouseCapture();
    }

    if (mouseCaptured_) {
        ProcessCameraInput();
        ProcessMovementInput();
    }
}

void PlayerController::ProcessCameraInput() {
    auto& input = InputManager::Get();

    float deltaX = input.GetAxis(inputConfig_.lookX);
    float deltaY = input.GetAxis(inputConfig_.lookY);

    float yInvert = cameraConfig_.invertY ? -1.0f : 1.0f;
    
    // Match old CameraController behavior: Yaw -= deltaX, Pitch -= deltaY
    AddControlRotation(
        -deltaX * cameraConfig_.sensitivityX,
        -deltaY * cameraConfig_.sensitivityY * yInvert
    );
}

void PlayerController::ProcessMovementInput() {
    if (!pawn_) return;
    
    auto& input = InputManager::Get();

    float moveX = input.GetAxis(inputConfig_.moveRight);
    float moveZ = input.GetAxis(inputConfig_.moveForward);

    // Add movement input to pawn (pawn will process in camera-relative space)
    if (std::abs(moveX) > 0.01f || std::abs(moveZ) > 0.01f) {
        pawn_->AddMovementInput(Vector3{moveX, 0.0f, moveZ});
    }

    // Jump
    if (input.IsActionJustPressed(inputConfig_.jump)) {
        if (auto* character = GetPawn<Character>()) {
            character->Jump();
        }
    }
}

void PlayerController::UpdateCamera(float dt) {
    if (!camera_ || !pawn_) return;
    
    Entity pawnEntity = pawn_->GetEntity();
    if (!pawnEntity.HasComponent<TransformComponent>()) return;
    
    auto& transform = pawnEntity.GetComponent<TransformComponent>();

    // Calculate camera direction from control rotation
    float yaw = glm::radians(controlRotation_.y);
    float pitch = glm::radians(controlRotation_.x);

    Vector3 direction{
        std::cos(pitch) * std::sin(yaw),
        std::sin(pitch),
        std::cos(pitch) * std::cos(yaw)
    };

    // Calculate camera right vector for socket offset
    Vector3 cameraRight{std::cos(yaw), 0.0f, -std::sin(yaw)};
    Vector3 cameraUp{0.0f, 1.0f, 0.0f};

    // Apply socket offset in camera local space
    Vector3 worldOffset = cameraRight * cameraConfig_.socketOffset.x 
                        + cameraUp * cameraConfig_.socketOffset.y;

    Vector3 targetPos = transform.Position + worldOffset;

    // Calculate arm length with collision
    float desiredLength = CalculateArmLengthWithCollision(targetPos, direction);

    // Smooth lerp
    float lerpSpeed = (desiredLength < currentArmLength_)
                          ? cameraConfig_.lagSpeedIn
                          : cameraConfig_.lagSpeedOut;

    currentArmLength_ = glm::mix(
        currentArmLength_,
        desiredLength,
        glm::clamp(lerpSpeed * dt, 0.0f, 1.0f)
    );

    // Apply camera transform
    Vector3 camPos = targetPos + direction * currentArmLength_;
    camera_->SetPosition(camPos);
    camera_->SetYaw(-controlRotation_.y - 90.0f);
    camera_->SetPitch(-controlRotation_.x);
}

float PlayerController::CalculateArmLengthWithCollision(const Vector3& targetPos,
                                                         const Vector3& direction) {
    float desired = cameraConfig_.targetArmLength;

    if (!cameraConfig_.doCollisionTest || !physicsSystem_) {
        return desired;
    }

    // Get pawn's rigidbody to ignore in raycast
    btRigidBody* ignoredBody = nullptr;
    if (pawn_) {
        auto* rb = pawn_->GetEntity().FindComponent<RigidbodyComponent>();
        if (rb) {
            ignoredBody = rb->GetRigidbody();
        }
    }

    Vector3 rayEnd = targetPos + direction * (desired + cameraConfig_.probeRadius);
    Vector3 hitPoint, hitNormal;

    bool hit = physicsSystem_->Raycast(targetPos, rayEnd, hitPoint, hitNormal, ignoredBody);

    if (hit) {
        float hitDist = glm::length(hitPoint - targetPos) - cameraConfig_.probeRadius;
        return glm::max(hitDist, cameraConfig_.minArmLength);
    }

    return desired;
}

void PlayerController::SetMouseCaptured(bool captured) {
    mouseCaptured_ = captured;

    auto* window = Application::Get().GetWindow().GetNativeWindow();
    glfwSetInputMode(window, GLFW_CURSOR,
                     captured ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);

    SE_LOG_INFO("[PlayerController] Mouse capture: {}", captured ? "enabled" : "disabled");
}

void PlayerController::ToggleMouseCapture() {
    SetMouseCaptured(!mouseCaptured_);
}

Vector3 PlayerController::GetForwardDirection() const {
    float yaw = glm::radians(controlRotation_.y);
    return Vector3{-std::sin(yaw), 0.0f, -std::cos(yaw)};
}

Vector3 PlayerController::GetRightDirection() const {
    float yaw = glm::radians(controlRotation_.y);
    return Vector3{std::cos(yaw), 0.0f, -std::sin(yaw)};
}

}  // namespace se
