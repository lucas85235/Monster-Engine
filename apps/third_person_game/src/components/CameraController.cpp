#include "CameraController.h"

#include "engine/Application.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/physics/PhysicsSystem.h"
#include "engine/physics/RigidbodyComponent.h"

namespace FirstGame {



CameraController::~CameraController() {
    delete camera_;
    camera_ = nullptr;
}

void CameraController::Awake() {
    CreateCamera();
    SetupSpringArm();
}

void CameraController::Start() {
    CacheComponents();

    if (camera_ && GetScene()) {
        GetScene()->SetActiveCamera(camera_);
    }
}

void CameraController::LateUpdate(float dt) {
    UpdateCameraPosition(dt);
}



void CameraController::CreateCamera() {
    camera_ = new Camera(Vector3(0.0f, 5.0f, 10.0f));
}

void CameraController::SetupSpringArm() {
    auto& arm = GetEntity().AddComponent<SpringArmComponent>();

    arm.TargetArmLength  = springArmConfig_.targetArmLength;
    arm.CurrentArmLength = springArmConfig_.targetArmLength;
    arm.SocketOffset     = springArmConfig_.socketOffset;
    arm.Pitch            = springArmConfig_.initialPitch;
    arm.MinPitch         = springArmConfig_.minPitch;
    arm.MaxPitch         = springArmConfig_.maxPitch;
    arm.ProbeSize        = springArmConfig_.probeSize;
    arm.DoCollisionTest  = springArmConfig_.enableCollision;
}

void CameraController::CacheComponents() {
    Entity entity = GetEntity();

    if (entity.HasComponent<TransformComponent>()) {
        transform_ = &entity.GetComponent<TransformComponent>();
    }

    if (entity.HasComponent<SpringArmComponent>()) {
        springArm_ = &entity.GetComponent<SpringArmComponent>();
    }

    // Try to get rigidbody for collision ignore
    rigidbody_ = entity.FindComponent<RigidbodyComponent>();
}



void CameraController::RotateCamera(float deltaX, float deltaY) {
    if (!springArm_) return;

    float yInvert = inputConfig_.invertY ? -1.0f : 1.0f;

    springArm_->Yaw   -= deltaX * inputConfig_.sensitivityX;
    springArm_->Pitch -= deltaY * inputConfig_.sensitivityY * yInvert;
    springArm_->Pitch  = glm::clamp(springArm_->Pitch, springArm_->MinPitch, springArm_->MaxPitch);
}

float CameraController::GetCurrentYaw() const {
    return springArm_ ? springArm_->Yaw : 0.0f;
}

float CameraController::GetCurrentPitch() const {
    return springArm_ ? springArm_->Pitch : 0.0f;
}

Vector3 CameraController::GetForwardDirection() const {
    float yaw = glm::radians(GetCurrentYaw());
    return Vector3(-std::sin(yaw), 0.0f, -std::cos(yaw));
}

Vector3 CameraController::GetRightDirection() const {
    float yaw = glm::radians(GetCurrentYaw());
    return Vector3(std::cos(yaw), 0.0f, -std::sin(yaw));
}



void CameraController::UpdateCameraPosition(float dt) {
    if (!springArm_ || !transform_ || !camera_) return;

    Vector3 direction = CalculateCameraDirection();
    Vector3 targetPos = transform_->Position + springArm_->SocketOffset;

    // Calculate arm length with collision
    float desiredLength = CalculateArmLengthWithCollision(targetPos, direction);

    // Smooth lerp (faster when moving in, slower when moving out)
    float lerpSpeed = (desiredLength < springArm_->CurrentArmLength)
                          ? springArmConfig_.lerpSpeedIn
                          : springArmConfig_.lerpSpeedOut;

    springArm_->CurrentArmLength = glm::mix(
        springArm_->CurrentArmLength,
        desiredLength,
        glm::clamp(lerpSpeed * dt, 0.0f, 1.0f)
    );

    // Apply camera transform
    Vector3 camPos = targetPos + direction * springArm_->CurrentArmLength;
    camera_->SetPosition(camPos);
    camera_->SetYaw(-springArm_->Yaw - 90.0f);
    camera_->SetPitch(-springArm_->Pitch);
}

Vector3 CameraController::CalculateCameraDirection() const {
    float yaw   = glm::radians(springArm_->Yaw);
    float pitch = glm::radians(springArm_->Pitch);

    return Vector3(
        std::cos(pitch) * std::sin(yaw),
        std::sin(pitch),
        std::cos(pitch) * std::cos(yaw)
    );
}

float CameraController::CalculateArmLengthWithCollision(const Vector3& targetPos,
                                                        const Vector3& direction) {
    float desired = springArm_->TargetArmLength;

    if (!springArm_->DoCollisionTest) return desired;

    Scene* scene = GetScene();
    if (!scene || !scene->GetPhysicsSystem()) return desired;

    // Ignore player body in raycast
    btRigidBody* ignoredBody = rigidbody_ ? rigidbody_->GetRigidbody() : nullptr;

    Vector3 rayEnd = targetPos + direction * (desired + springArm_->ProbeSize);
    Vector3 hitPoint, hitNormal;

    bool hit = scene->GetPhysicsSystem()->Raycast(
        targetPos, rayEnd, hitPoint, hitNormal, ignoredBody);

    if (hit) {
        float hitDist = glm::length(hitPoint - targetPos) - springArm_->ProbeSize;
        return glm::max(hitDist, springArmConfig_.minArmLength);
    }

    return desired;
}

} // namespace FirstGame
