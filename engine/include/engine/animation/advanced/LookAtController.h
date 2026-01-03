#pragma once
/**
 * LookAtController.h - Procedural look-at with distributed bone rotation.
 * 
 * Distributes camera-following rotation across spine chain for natural movement.
 * Supports configurable bone weights and rotation limits per joint.
 */

#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include <vector>
#include <string>

namespace se {

class SkinnedModelData;

namespace anim {

class Pose;

struct LookAtBoneSettings {
    std::string boneName;
    float horizontalWeight = 0.2f;
    float verticalWeight = 0.2f;
    float maxHorizontalDegrees = 30.0f;
    float maxVerticalDegrees = 20.0f;
};

struct LookAtSettings {
    std::vector<LookAtBoneSettings> boneChain;
    float smoothSpeed = 10.0f;
    float horizontalLimit = 90.0f;
    float verticalLimit = 60.0f;
    float deadzone = 5.0f;
    bool enabled = true;
    
    static LookAtSettings DefaultMixamo();
};

class LookAtController {
public:
    LookAtController() = default;
    
    void Initialize(const SkinnedModelData* skeleton, const LookAtSettings& settings);
    
    void SetTarget(const glm::vec3& worldDirection, const glm::vec3& characterForward, const glm::vec3& characterUp);
    
    void SetTargetAngles(float horizontalDegrees, float verticalDegrees);
    
    void Update(float dt);
    
    void ApplyToPose(Pose& pose);
    
    void SetEnabled(bool enabled) { settings_.enabled = enabled; }
    bool IsEnabled() const { return settings_.enabled; }
    
    void SetSmoothSpeed(float speed) { settings_.smoothSpeed = speed; }
    float GetSmoothSpeed() const { return settings_.smoothSpeed; }
    
    glm::vec2 GetCurrentAngles() const { return currentAngles_; }
    glm::vec2 GetTargetAngles() const { return targetAngles_; }
    
    LookAtSettings& GetSettings() { return settings_; }
    const LookAtSettings& GetSettings() const { return settings_; }
    
    bool IsInitialized() const { return initialized_; }
    
private:
    struct ResolvedBone {
        int index = -1;
        float horizontalWeight = 0.0f;
        float verticalWeight = 0.0f;
        float maxHorizontal = 0.0f;
        float maxVertical = 0.0f;
    };
    
    LookAtSettings settings_;
    std::vector<ResolvedBone> resolvedBones_;
    const SkinnedModelData* skeleton_ = nullptr;
    
    glm::vec2 currentAngles_{0.0f};
    glm::vec2 targetAngles_{0.0f};
    
    bool initialized_ = false;
};

}  // namespace anim
}  // namespace se
