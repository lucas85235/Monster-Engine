#include "apps/third_person_game/src/CharacterController.h"

#include "engine/Application.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/physics/PhysicsSystem.h"
#include "engine/physics/RigidbodyComponent.h"

namespace FirstGame {

void CharacterController::Awake() {
    SE_LOG_INFO("CharacterController::Awake() - Binding input");
    BindInput();
}

void CharacterController::Start() {
    SE_LOG_INFO("CharacterController::Start() - Controller ready for entity {}",
                GetEntity().GetID());
}

void CharacterController::Update(float dt) {
    UpdateInputs();
    UpdateCamera(dt);

    SE_LOG_CRITICAL("Character controller update");
}

void CharacterController::UpdateCamera(float dt) {
    if (!mouseCaptured_) return;

    Entity entity = GetEntity();
    auto&  input  = InputManager::Get();

    if (!entity.HasComponent<SpringArmComponent>()) return;
    auto& springArm   = entity.GetComponent<SpringArmComponent>();
    auto& playerTrans = entity.GetComponent<TransformComponent>();

    float mouseX = input.GetAxis("CameraRotateX");
    float mouseY = input.GetAxis("CameraRotateY");

    springArm.Yaw -= mouseX * 0.1f;
    springArm.Pitch -= mouseY * 0.1f;
    springArm.Pitch = glm::clamp(springArm.Pitch, springArm.MinPitch, springArm.MaxPitch);

    float yawRad   = glm::radians(springArm.Yaw);
    float pitchRad = glm::radians(springArm.Pitch);

    float sinYaw   = std::sin(yawRad);
    float cosYaw   = std::cos(yawRad);
    float sinPitch = std::sin(pitchRad);
    float cosPitch = std::cos(pitchRad);

    glm::vec3 direction;
    direction.x = cosPitch * sinYaw;
    direction.y = sinPitch;
    direction.z = cosPitch * cosYaw;

    glm::vec3 targetPos = playerTrans.Position + springArm.SocketOffset;

    float desiredArmLength = springArm.TargetArmLength;

    Scene* scene = GetScene();
    if (springArm.DoCollisionTest && scene && scene->GetPhysicsSystem()) {
        btRigidBody* playerBody = nullptr;
        if (entity.HasComponent<RigidbodyComponent>()) {
            playerBody = entity.GetComponent<RigidbodyComponent>().GetRigidbody();
        }

        glm::vec3 rayStart = targetPos;
        glm::vec3 rayEnd =
            targetPos + direction * (springArm.TargetArmLength + springArm.ProbeSize);
        glm::vec3 hitPoint, hitNormal;

        bool hit =
            scene->GetPhysicsSystem()->Raycast(rayStart, rayEnd, hitPoint, hitNormal, playerBody);

        if (hit) {
            float hitDistance = glm::length(hitPoint - rayStart) - springArm.ProbeSize;
            desiredArmLength  = glm::max(hitDistance, 0.5f);
        }
    }

    float lerpSpeed            = (desiredArmLength < springArm.CurrentArmLength) ? 15.0f : 5.0f;
    springArm.CurrentArmLength = glm::mix(springArm.CurrentArmLength, desiredArmLength,
                                          glm::clamp(lerpSpeed * dt, 0.0f, 1.0f));

    glm::vec3 camPos = targetPos + direction * springArm.CurrentArmLength;

    camera_.SetPosition(camPos);
    camera_.SetYaw(-springArm.Yaw - 90.0f);
    camera_.SetPitch(-springArm.Pitch);
}

void CharacterController::BindInput() {
    auto& input = InputManager::Get();
    input.BindAction("ToggleMouse", Key::Tab);
    input.BindAxis("CameraRotateX", Key::MouseX, 1.0f);
    input.BindAxis("CameraRotateY", Key::MouseY, -1.0f);
    SE_LOG_CRITICAL("CharacterController: Input binded");
}

void CharacterController::UpdateInputs() {
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

}  // namespace FirstGame
