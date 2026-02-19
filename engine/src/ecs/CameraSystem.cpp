#include "engine/ecs/CameraSystem.h"

#include <cmath>

#include "engine/Application.h"
#include "engine/Log.h"
#include "engine/core/ServiceLocator.h"
#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/input/InputManager.h"
#include "engine/physics/PhysicsSystem.h"
#include "engine/renderer/FilamentRenderer.h"

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

namespace se {

namespace {

// ─── FreeFly mode ────────────────────────────────────────────────────
void UpdateFreeFly(CameraComponent& cam, TransformComponent& transform,
                   InputManager& input, float dt) {
    // Mouse look
    float mouseX = input.GetAxis("LookX");
    float mouseY = input.GetAxis("LookY");

    cam.Yaw += mouseX * cam.LookSensitivityX;
    float pitchDelta = mouseY * cam.LookSensitivityY;
    if (cam.InvertY) pitchDelta = -pitchDelta;
    cam.Pitch += pitchDelta;
    cam.Pitch = glm::clamp(cam.Pitch, -89.0f, 89.0f);

    // Calculate direction vectors
    float yawRad   = glm::radians(cam.Yaw);
    float pitchRad = glm::radians(cam.Pitch);

    Vector3 front;
    front.x = cos(yawRad) * cos(pitchRad);
    front.y = sin(pitchRad);
    front.z = sin(yawRad) * cos(pitchRad);
    front = glm::normalize(front);

    Vector3 worldUp(0.0f, 1.0f, 0.0f);
    Vector3 right = glm::normalize(glm::cross(front, worldUp));
    Vector3 up    = glm::normalize(glm::cross(right, front));

    // WASD movement
    float moveForward = input.GetAxis("MoveForward");
    float moveRight   = input.GetAxis("MoveRight");
    float moveUp      = input.GetAxis("MoveUp");

    bool sprinting = input.IsActionPressed("Sprint");
    float speed = sprinting ? cam.SprintSpeed : cam.MoveSpeed;

    Vector3 velocity(0.0f);
    velocity += front * moveForward * speed * dt;
    velocity += right * moveRight * speed * dt;
    velocity += worldUp * moveUp * speed * dt;

    transform.Translate(velocity);
}

// ─── ThirdPerson mode ────────────────────────────────────────────────
void UpdateThirdPerson(CameraComponent& cam, TransformComponent& camTransform,
                       SpringArmComponent& springArm, entt::registry& registry,
                       PhysicsSystem* physics, InputManager& input, float /*dt*/) {
    // Mouse look updates spring arm angles
    float mouseX = input.GetAxis("LookX");
    float mouseY = input.GetAxis("LookY");

    springArm.Yaw -= mouseX * cam.LookSensitivityX;

    float pitchDelta = mouseY * cam.LookSensitivityY;
    if (cam.InvertY) pitchDelta = -pitchDelta;
    springArm.Pitch -= pitchDelta;
    springArm.Pitch = glm::clamp(springArm.Pitch, springArm.MinPitch, springArm.MaxPitch);

    // Get target entity position
    Vector3 targetPos(0.0f);
    if (cam.TargetEntity != entt::null && registry.valid(cam.TargetEntity)) {
        if (registry.any_of<TransformComponent>(cam.TargetEntity)) {
            targetPos = registry.get<TransformComponent>(cam.TargetEntity).Position;
        }
    }

    targetPos += springArm.SocketOffset;

    // Calculate camera position from spring arm
    float yawRad   = glm::radians(springArm.Yaw);
    float pitchRad = glm::radians(springArm.Pitch);

    Vector3 direction;
    direction.x = cos(pitchRad) * sin(yawRad);
    direction.y = sin(pitchRad);
    direction.z = cos(pitchRad) * cos(yawRad);

    float desiredArmLength = springArm.TargetArmLength;

    // Spring arm collision test
    if (springArm.DoCollisionTest && physics) {
        Vector3 rayStart = targetPos;
        Vector3 rayEnd   = targetPos + direction * (springArm.TargetArmLength + 0.5f);
        Vector3 hitPoint, hitNormal;

        bool hit = physics->Raycast(rayStart, rayEnd, hitPoint, hitNormal, nullptr);
        if (hit) {
            float hitDistance = glm::length(hitPoint - rayStart) - 0.5f;
            desiredArmLength  = glm::max(hitDistance, 0.5f);
        }
    }

    // Smooth arm length transitions
    float lerpSpeed = (desiredArmLength < springArm.CurrentArmLength) ? 15.0f : 5.0f;
    float t = glm::clamp(lerpSpeed * (1.0f / 60.0f), 0.0f, 1.0f);
    springArm.CurrentArmLength = glm::mix(springArm.CurrentArmLength, desiredArmLength, t);

    Vector3 camPos = targetPos + direction * springArm.CurrentArmLength;
    camTransform.SetPosition(camPos);

    // Store yaw/pitch for external consumers
    cam.Yaw   = springArm.Yaw;
    cam.Pitch = springArm.Pitch;
}

// ─── Orbit mode ──────────────────────────────────────────────────────
void UpdateOrbit(CameraComponent& cam, TransformComponent& camTransform,
                 SpringArmComponent& springArm, entt::registry& registry,
                 InputManager& input, float /*dt*/) {
    // Use same spring arm concept but for orbit
    float mouseX = input.GetAxis("LookX");
    float mouseY = input.GetAxis("LookY");

    springArm.Yaw -= mouseX * cam.LookSensitivityX;
    float pitchDelta = mouseY * cam.LookSensitivityY;
    if (cam.InvertY) pitchDelta = -pitchDelta;
    springArm.Pitch -= pitchDelta;
    springArm.Pitch = glm::clamp(springArm.Pitch, springArm.MinPitch, springArm.MaxPitch);

    // Scroll wheel distance
    float scroll = input.GetAxis("ScrollWheel");
    if (std::abs(scroll) > 0.01f) {
        springArm.TargetArmLength -= scroll * 0.5f;
        springArm.TargetArmLength = glm::clamp(springArm.TargetArmLength, 1.0f, 100.0f);
    }

    // Target position
    Vector3 targetPos(0.0f);
    if (cam.TargetEntity != entt::null && registry.valid(cam.TargetEntity)) {
        if (registry.any_of<TransformComponent>(cam.TargetEntity)) {
            targetPos = registry.get<TransformComponent>(cam.TargetEntity).Position;
        }
    }
    targetPos += springArm.SocketOffset;

    float yawRad   = glm::radians(springArm.Yaw);
    float pitchRad = glm::radians(springArm.Pitch);

    Vector3 direction;
    direction.x = cos(pitchRad) * sin(yawRad);
    direction.y = sin(pitchRad);
    direction.z = cos(pitchRad) * cos(yawRad);

    springArm.CurrentArmLength = springArm.TargetArmLength;

    Vector3 camPos = targetPos + direction * springArm.CurrentArmLength;
    camTransform.SetPosition(camPos);

    cam.Yaw   = springArm.Yaw;
    cam.Pitch = springArm.Pitch;
}

// ─── Sync camera to Filament ─────────────────────────────────────────
void SyncToFilament(const CameraComponent& cam, const TransformComponent& transform,
                    const SpringArmComponent* springArm, entt::registry& registry) {
    auto& renderer = ServiceLocator::Get().GetFilamentRenderer();
    auto& window   = Application::Get().GetWindow();
    float aspect   = static_cast<float>(window.GetWidth()) /
                     static_cast<float>(window.GetHeight());

    renderer.SetCameraProjection(cam.FOV, aspect, cam.NearPlane, cam.FarPlane);

    Vector3 camPos = transform.Position;

    if (cam.Mode == CameraMode::ThirdPerson || cam.Mode == CameraMode::Orbit) {
        // Look at target
        Vector3 targetPos(0.0f);
        if (cam.TargetEntity != entt::null && registry.valid(cam.TargetEntity)) {
            if (registry.any_of<TransformComponent>(cam.TargetEntity)) {
                targetPos = registry.get<TransformComponent>(cam.TargetEntity).Position;
            }
        }
        if (springArm) {
            targetPos += springArm->SocketOffset;
        }

        luma::Vector3 eye    = {camPos.x, camPos.y, camPos.z};
        luma::Vector3 center = {targetPos.x, targetPos.y, targetPos.z};
        luma::Vector3 up     = {0.0f, 1.0f, 0.0f};
        renderer.SetCameraLookAt(eye, center, up);
    } else {
        // FreeFly / Fixed: look-at from yaw/pitch
        float yawRad   = glm::radians(cam.Yaw);
        float pitchRad = glm::radians(cam.Pitch);

        Vector3 front;
        front.x = cos(yawRad) * cos(pitchRad);
        front.y = sin(pitchRad);
        front.z = sin(yawRad) * cos(pitchRad);
        front = glm::normalize(front);

        Vector3 target = camPos + front;

        luma::Vector3 eye    = {camPos.x, camPos.y, camPos.z};
        luma::Vector3 center = {target.x, target.y, target.z};
        luma::Vector3 up     = {0.0f, 1.0f, 0.0f};
        renderer.SetCameraLookAt(eye, center, up);
    }
}

}  // anonymous namespace

void CameraSystem::Update(Scene& scene, float dt) {
    auto& registry = scene.GetRegistry();

    // Process all camera entities
    auto view = registry.view<CameraComponent, TransformComponent>();

    for (auto entity : view) {
        auto& cam       = view.get<CameraComponent>(entity);
        auto& transform = view.get<TransformComponent>(entity);

        if (!cam.IsMain) continue;

        auto& input = InputManager::Get();

        switch (cam.Mode) {
            case CameraMode::FreeFly:
                UpdateFreeFly(cam, transform, input, dt);
                break;

            case CameraMode::ThirdPerson: {
                SpringArmComponent* springArm = nullptr;
                if (registry.any_of<SpringArmComponent>(entity)) {
                    springArm = &registry.get<SpringArmComponent>(entity);
                }
                if (springArm) {
                    UpdateThirdPerson(cam, transform, *springArm, registry,
                                     scene.GetPhysicsSystem(), input, dt);
                }
                break;
            }

            case CameraMode::Orbit: {
                SpringArmComponent* springArm = nullptr;
                if (registry.any_of<SpringArmComponent>(entity)) {
                    springArm = &registry.get<SpringArmComponent>(entity);
                }
                if (springArm) {
                    UpdateOrbit(cam, transform, *springArm, registry, input, dt);
                }
                break;
            }

            case CameraMode::Fixed:
                // No input processing — camera stays where placed
                break;
        }

        // Sync to Filament renderer
        SpringArmComponent* springArm = nullptr;
        if (registry.any_of<SpringArmComponent>(entity)) {
            springArm = &registry.get<SpringArmComponent>(entity);
        }
        SyncToFilament(cam, transform, springArm, registry);

        // Only process the first main camera
        break;
    }
}

}  // namespace se
