#include "engine/animation/advanced/BodyRotationController.h"

#include "engine/ecs/SimpleComponents.h"
#include "engine/Log.h"

#include <cmath>

namespace se::anim {

void BodyRotationController::Initialize(const BodyRotationSettings& settings) {
    settings_ = settings;
    angularVelocity_ = 0.0f;
    isRotating_ = false;
    
    SE_LOG_INFO("[BodyRotationController] Initialized (activation: {}°, deactivation: {}°)",
                settings_.activationThreshold, settings_.deactivationThreshold);
}

float BodyRotationController::NormalizeAngle(float angle) {
    while (angle > 180.0f) angle -= 360.0f;
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

float BodyRotationController::ShortestAngleDifference(float from, float to) {
    float diff = NormalizeAngle(to - from);
    return diff;
}

void BodyRotationController::Update(float dt, float targetYaw) {
    if (!settings_.enabled) {
        return;
    }
    
    // Calculate angle difference
    angleDifference_ = ShortestAngleDifference(currentYaw_, targetYaw);
    float absDiff = std::abs(angleDifference_);
    
    // Hysteresis pattern
    if (!isRotating_) {
        // Check if we should start rotating
        if (absDiff > settings_.activationThreshold) {
            isRotating_ = true;
        }
    } else {
        // Check if we should stop rotating
        if (absDiff < settings_.deactivationThreshold) {
            isRotating_ = false;
        }
    }
    
    // Apply deadzone
    if (absDiff < settings_.deadzone) {
        // Gradually stop
        angularVelocity_ *= 0.9f;
        if (std::abs(angularVelocity_) < 1.0f) {
            angularVelocity_ = 0.0f;
        }
        return;
    }
    
    if (!isRotating_) {
        // Dampen velocity when not actively rotating
        angularVelocity_ *= (1.0f - settings_.damping * dt * 2.0f);
        return;
    }
    
    // Spring-damper physics
    // F = -k * x - c * v
    // Acceleration = -stiffness * displacement - damping * velocity
    float displacement = -angleDifference_;  // Negative because we want to move toward target
    float acceleration = -settings_.stiffness * displacement - settings_.damping * angularVelocity_;
    
    // Update velocity
    angularVelocity_ += acceleration * dt;
    
    // Clamp velocity
    float maxVel = settings_.maxAngularVelocity;
    angularVelocity_ = glm::clamp(angularVelocity_, -maxVel, maxVel);
    
    // Update position
    currentYaw_ += angularVelocity_ * dt;
    currentYaw_ = NormalizeAngle(currentYaw_);
}

void BodyRotationController::ApplyToTransform(TransformComponent& transform) {
    if (!settings_.enabled) {
        return;
    }
    
    // Get current rotation
    glm::vec3 rotation = transform.Rotation;
    
    // Apply yaw rotation
    rotation.y = currentYaw_;
    
    transform.SetRotation(rotation);
}

}  // namespace se::anim
