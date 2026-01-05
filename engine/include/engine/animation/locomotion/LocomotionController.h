#pragma once

#include "engine/animation/locomotion/LocomotionConfig.h"
#include "engine/animation/advanced/BlendSpace.h"
#include "engine/animation/advanced/Pose.h"

#include <memory>
#include <functional>

namespace se {

class SkinnedModelData;

namespace anim {

enum class LocomotionMode {
    Standing,   // Normal movement with full body rotation
    Aiming,     // Strafe movement with locked forward direction
    Swimming,   // Future: swimming animations
    Custom      // User-defined mode
};

class LocomotionController {
public:
    LocomotionController() = default;
    
    void Initialize(const LocomotionConfig& config, const SkinnedModelData* skeleton);
    bool IsInitialized() const { return initialized_; }
    
    void SetMode(LocomotionMode mode);
    LocomotionMode GetMode() const { return currentMode_; }
    bool IsInTransition() const { return modeTransitionWeight_ < 1.0f && modeTransitionWeight_ > 0.0f; }
    
    void SetVelocity(float speed);
    float GetVelocity() const { return currentVelocity_; }
    float GetTargetVelocity() const { return targetVelocity_; }
    
    void SetStrafeInput(const glm::vec2& input);
    glm::vec2 GetStrafeInput() const { return strafeInput_; }
    
    void Update(float dt, Pose& outPose);
    
    // Event callbacks
    using ModeChangedCallback = std::function<void(LocomotionMode oldMode, LocomotionMode newMode)>;
    void OnModeChanged(ModeChangedCallback callback) { modeChangedCallback_ = callback; }
    
    // Access blend spaces for debug/visualization
    BlendSpace1D* GetLocomotionBlendSpace() { return locomotionBlendSpace_.get(); }
    BlendSpace2D* GetStrafeBlendSpace() { return strafeBlendSpace_.get(); }
    const BlendSpace1D* GetLocomotionBlendSpace() const { return locomotionBlendSpace_.get(); }
    const BlendSpace2D* GetStrafeBlendSpace() const { return strafeBlendSpace_.get(); }
    
    LocomotionConfig& GetConfig() { return config_; }
    const LocomotionConfig& GetConfig() const { return config_; }
    
private:
    void SetupBlendSpaces();
    void UpdateVelocity(float dt);
    void UpdateModeTransition(float dt);
    
    LocomotionConfig config_;
    const SkinnedModelData* skeleton_ = nullptr;
    bool initialized_ = false;
    
    // Blend spaces
    std::unique_ptr<BlendSpace1D> locomotionBlendSpace_;
    std::unique_ptr<BlendSpace2D> strafeBlendSpace_;
    
    // Mode
    LocomotionMode currentMode_ = LocomotionMode::Standing;
    LocomotionMode targetMode_ = LocomotionMode::Standing;
    float modeTransitionWeight_ = 1.0f;
    
    // Velocity state
    float currentVelocity_ = 0.0f;
    float targetVelocity_ = 0.0f;
    glm::vec2 strafeInput_{0.0f};
    glm::vec2 currentStrafeInput_{0.0f};
    
    // Animation time
    float animationTime_ = 0.0f;
    
    // Intermediate poses
    Pose standingPose_;
    Pose strafePose_;
    
    ModeChangedCallback modeChangedCallback_;
};

}  // namespace anim
}  // namespace se
