#include "CharacterController.h"

#include "engine/Application.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/physics/PhysicsSystem.h"
#include "engine/physics/RigidbodyComponent.h"

namespace FirstGame {
// ============================================================================
// Lifecycle Methods
// ============================================================================

void CharacterController::Awake() {
    SE_LOG_INFO("CharacterController::Awake()");

    InitializeCamera();
    InitializeSpringArm();
    BindInputActions();
}

void CharacterController::Start() {
    // Cache component references for performance
    Entity entity = GetEntity();

    if (entity.HasComponent<TransformComponent>()) {
        transform_ = &entity.GetComponent<TransformComponent>();
    }

    // RigidbodyComponent inherits from Component (lifecycle), so use GetScript
    if (entity.HasScript<RigidbodyComponent>()) {
        rigidbody_ = entity.GetScript<RigidbodyComponent>();
        SE_LOG_INFO("CharacterController: Rigidbody found with body ptr: {}",
                    (void*)rigidbody_->GetRigidbody());
    } else {
        SE_LOG_WARN("CharacterController: No RigidbodyComponent found on entity!");
    }

    if (entity.HasComponent<SpringArmComponent>()) {
        springArm_ = &entity.GetComponent<SpringArmComponent>();
    }

    // Set this camera as the active camera for the scene
    if (camera_) {
        GetScene()->SetActiveCamera(camera_);
        SE_LOG_INFO("CharacterController: Camera set as active");
    }

    SE_LOG_INFO("CharacterController::Start() - Ready for entity {}", GetEntityID());
}

void CharacterController::Update(float dt) {
    UpdateInputState();
    UpdateMovement(dt);
    UpdateCameraPosition(dt);
}

// ============================================================================
// Initialization
// ============================================================================

void CharacterController::InitializeCamera() {
    camera_ = new Camera(glm::vec3(0.0f, 5.0f, 10.0f));
}

void CharacterController::InitializeSpringArm() {
    auto& springArm           = GetEntity().AddComponent<SpringArmComponent>();
    springArm.TargetArmLength = 8.0f;
    springArm.SocketOffset    = {0.0f, 1.5f, 0.0f};
    springArm.Pitch           = -30.0f;
}

void CharacterController::BindInputActions() {
    auto& input = InputManager::Get();

    // Camera rotation
    input.BindAction("ToggleMouse", Key::Tab);
    input.BindAxis("CameraRotateX", Key::MouseX, 1.0f);
    input.BindAxis("CameraRotateY", Key::MouseY, -1.0f);

    // Movement
    input.BindAxis("MoveForward", Key::W, 1.0f);
    input.BindAxis("MoveForward", Key::S, -1.0f);
    input.BindAxis("MoveRight", Key::D, 1.0f);
    input.BindAxis("MoveRight", Key::A, -1.0f);
}

// ============================================================================
// Update Logic
// ============================================================================

void CharacterController::UpdateInputState() {
    HandleMouseCapture();
}

void CharacterController::UpdateMovement(float dt) {
    if (!mouseCaptured_ || !rigidbody_ || !springArm_) return;

    glm::vec2 moveInput = GetMovementInput();
    if (glm::length(moveInput) < 0.01f) return;

    // Calculate camera-relative directions
    float     yawRad     = glm::radians(springArm_->Yaw);
    glm::vec3 camForward = {-std::sin(yawRad), 0.0f, -std::cos(yawRad)};
    glm::vec3 camRight   = {std::cos(yawRad), 0.0f, -std::sin(yawRad)};

    ApplyMovementVelocity(moveInput, camForward, camRight);
}

void CharacterController::UpdateCameraPosition(float dt) {
    if (!mouseCaptured_ || !springArm_ || !transform_) return;

    // Get and apply camera rotation input
    glm::vec2 rotInput = GetCameraRotationInput();
    UpdateSpringArmRotation(rotInput.x, rotInput.y);

    // Calculate camera direction
    float yawRad   = glm::radians(springArm_->Yaw);
    float pitchRad = glm::radians(springArm_->Pitch);

    glm::vec3 direction;
    CalculateCameraDirection(yawRad, pitchRad, direction);

    // Calculate target position
    glm::vec3 targetPos = transform_->Position + springArm_->SocketOffset;

    // Calculate arm length with collision detection
    float desiredArmLength = CalculateArmLengthWithCollision(targetPos, direction);

    // Smooth arm length transition
    float lerpSpeed = (desiredArmLength < springArm_->CurrentArmLength)
                          ? cameraLerpSpeedIn_
                          : cameraLerpSpeedOut_;
    springArm_->CurrentArmLength = glm::mix(springArm_->CurrentArmLength, desiredArmLength,
                                            glm::clamp(lerpSpeed * dt, 0.0f, 1.0f));

    // Apply final camera transform
    glm::vec3 camPos = targetPos + direction * springArm_->CurrentArmLength;
    ApplyCameraTransform(camPos, springArm_->Yaw, springArm_->Pitch);
}

// ============================================================================
// Input Helpers
// ============================================================================

void CharacterController::HandleMouseCapture() {
    auto& input = InputManager::Get();

    if (input.IsActionJustPressed("ToggleMouse")) {
        auto& app      = Application::Get();
        auto* window   = app.GetWindow().GetNativeWindow();
        mouseCaptured_ = !mouseCaptured_;
        glfwSetInputMode(window, GLFW_CURSOR,
                         mouseCaptured_ ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        SE_LOG_INFO("Mouse capture: {}", mouseCaptured_ ? "enabled" : "disabled");
    }
}

glm::vec2 CharacterController::GetCameraRotationInput() const {
    auto& input = InputManager::Get();
    return {input.GetAxis("CameraRotateX"), input.GetAxis("CameraRotateY")};
}

glm::vec2 CharacterController::GetMovementInput() const {
    auto& input = InputManager::Get();
    return {input.GetAxis("MoveRight"), input.GetAxis("MoveForward")};
}

// ============================================================================
// Camera Helpers
// ============================================================================

void CharacterController::UpdateSpringArmRotation(float mouseX, float mouseY) {
    springArm_->Yaw -= mouseX * cameraSensitivity_;
    springArm_->Pitch -= mouseY * cameraSensitivity_;
    springArm_->Pitch = glm::clamp(springArm_->Pitch, springArm_->MinPitch, springArm_->MaxPitch);
}

void CharacterController::CalculateCameraDirection(float      yawRad, float pitchRad,
                                                   glm::vec3& outDirection) const {
    float sinYaw   = std::sin(yawRad);
    float cosYaw   = std::cos(yawRad);
    float sinPitch = std::sin(pitchRad);
    float cosPitch = std::cos(pitchRad);

    outDirection.x = cosPitch * sinYaw;
    outDirection.y = sinPitch;
    outDirection.z = cosPitch * cosYaw;
}

float CharacterController::CalculateArmLengthWithCollision(const glm::vec3& targetPos,
                                                           const glm::vec3& direction) {
    float desiredArmLength = springArm_->TargetArmLength;

    Scene* scene = GetScene();
    if (!springArm_->DoCollisionTest || !scene || !scene->GetPhysicsSystem()) {
        return desiredArmLength;
    }

    btRigidBody* playerBody = rigidbody_ ? rigidbody_->GetRigidbody() : nullptr;

    glm::vec3 rayStart = targetPos;
    glm::vec3 rayEnd   = targetPos + direction * (
                           springArm_->TargetArmLength + springArm_->ProbeSize);
    glm::vec3 hitPoint, hitNormal;

    bool hit = scene->GetPhysicsSystem()->
        Raycast(rayStart, rayEnd, hitPoint, hitNormal, playerBody);

    if (hit) {
        float hitDistance = glm::length(hitPoint - rayStart) - springArm_->ProbeSize;
        desiredArmLength  = glm::max(hitDistance, minArmLength_);
    }

    return desiredArmLength;
}

void CharacterController::ApplyCameraTransform(const glm::vec3& position, float yaw, float pitch) {
    if (!camera_) return;

    camera_->SetPosition(position);
    camera_->SetYaw(-yaw - 90.0f);
    camera_->SetPitch(-pitch);
}

// ============================================================================
// Movement Helpers
// ============================================================================

void CharacterController::ApplyMovementVelocity(const glm::vec2& moveInput,
                                                const glm::vec3& camForward,
                                                const glm::vec3& camRight) {
    if (!rigidbody_) return;

    Vector3   current_vertical_velocity(0.0f, rigidbody_->GetLinearVelocity().getY(), 0.0f);
    glm::vec3 movement = camForward * moveInput.y + camRight * moveInput.x;

    if (glm::length(movement) > 0.01f) {
        movement = glm::normalize(movement) * moveSpeed_ + current_vertical_velocity;
        rigidbody_->SetLinearVelocity(ToBt(movement));
    }
}
} // namespace FirstGame