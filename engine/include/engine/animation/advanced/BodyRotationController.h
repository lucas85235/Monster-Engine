#pragma once
/**
 * BodyRotationController.h - Natural body rotation with spring-damper physics.
 * 
 * Implements hysteresis pattern for smooth body rotation when camera exceeds
 * threshold angle from character forward direction.
 */

#include <glm.hpp>

namespace se {

struct TransformComponent;

namespace anim {

struct BodyRotationSettings {
    float activationThreshold = 45.0f;
    float deactivationThreshold = 30.0f;
    float deadzone = 10.0f;
    float stiffness = 5.0f;
    float damping = 0.7f;
    float maxAngularVelocity = 360.0f;
    bool enabled = true;
    
    static BodyRotationSettings Default() {
        return BodyRotationSettings{};
    }
};

class BodyRotationController {
public:
    BodyRotationController() = default;
    
    void Initialize(const BodyRotationSettings& settings);
    
    void Update(float dt, float targetYaw);
    
    float GetCurrentYaw() const { return currentYaw_; }
    void SetCurrentYaw(float yaw) { currentYaw_ = yaw; }
    
    bool IsRotating() const { return isRotating_; }
    
    float GetAngularVelocity() const { return angularVelocity_; }
    
    void ApplyToTransform(TransformComponent& transform);
    
    BodyRotationSettings& GetSettings() { return settings_; }
    const BodyRotationSettings& GetSettings() const { return settings_; }
    
    void SetEnabled(bool enabled) { settings_.enabled = enabled; }
    bool IsEnabled() const { return settings_.enabled; }
    
    float GetAngleDifference() const { return angleDifference_; }
    
private:
    float NormalizeAngle(float angle);
    float ShortestAngleDifference(float from, float to);
    
    BodyRotationSettings settings_;
    
    float currentYaw_ = 0.0f;
    float angularVelocity_ = 0.0f;
    float angleDifference_ = 0.0f;
    bool isRotating_ = false;
};

}  // namespace anim
}  // namespace se
