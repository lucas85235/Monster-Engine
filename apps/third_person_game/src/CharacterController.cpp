#include "apps/third_person_game/src/CharacterController.h"

#include "CharacterController.h"
#include "engine/Application.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/physics/PhysicsSystem.h"

namespace FirstGame {
CharacterController::~CharacterController() {}

void CharacterController::Update(float ts) {
    UpdateCamera();
}

void CharacterController::UpdateCamera() {
    if (!mouseCaptured_) return;
    auto& input = InputManager::Get();

    if (!character_->GetEntity().HasComponent<SpringArmComponent>()) return;
    auto& springArm   = character_->GetEntity().GetComponent<SpringArmComponent>();
    auto& playerTrans = character_->GetEntity().GetComponent<TransformComponent>();

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

    if (springArm.DoCollisionTest && scene_->GetPhysicsSystem()) {
        btRigidBody* playerBody = nullptr;
        if (character_->GetEntity().HasComponent<RigidbodyComponent>()) {
            playerBody = character_->GetEntity().GetComponent<RigidbodyComponent>().GetRigidbody();
        }

        glm::vec3 rayStart = targetPos;
        glm::vec3 rayEnd =
            targetPos + direction * (springArm.TargetArmLength + springArm.ProbeSize);
        glm::vec3 hitPoint, hitNormal;

        bool hit =
            scene_->GetPhysicsSystem()->Raycast(rayStart, rayEnd, hitPoint, hitNormal, playerBody);

        if (hit) {
            float hitDistance = glm::length(hitPoint - rayStart) - springArm.ProbeSize;
            desiredArmLength  = glm::max(hitDistance, 0.5f);
        }
    }

    float lerpSpeed            = (desiredArmLength < springArm.CurrentArmLength) ? 15.0f : 5.0f;
    springArm.CurrentArmLength = glm::mix(springArm.CurrentArmLength, desiredArmLength,
                                          glm::clamp(lerpSpeed * (1.0f / 60.0f), 0.0f, 1.0f));

    glm::vec3 camPos = targetPos + direction * springArm.CurrentArmLength;

    camera_.SetPosition(camPos);
    camera_.SetYaw(-springArm.Yaw - 90.0f);
    camera_.SetPitch(-springArm.Pitch);
}

}  // namespace FirstGame
