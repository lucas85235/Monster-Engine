#pragma once
/**
 * AdvancedCharacterAnimator - Advanced animation controller component.
 * 
 * Manages locomotion modes, blend spaces, animation layers, procedural
 * look-at, and body rotation for high-quality character animation.
 */

#include "engine/ecs/Component.h"
#include "engine/animation/advanced/AdvancedAnimation.h"
#include "engine/animation/AnimationClip.h"

#include <glm.hpp>
#include <memory>

namespace se {
class SkinnedModelData;
class AnimatorComponent;
}

namespace AnimationTest {

enum class LocomotionMode {
    Standing,   // Normal movement with full body rotation
    Aiming      // Strafe movement with locked forward direction
};

struct AdvancedCharacterAnimatorConfig {
    std::string modelPath = "assets/models/characters/Y_Bot.fbx";
    
    // Animation paths
    std::string idleAnimPath = "assets/models/characters/animations/YBot_Idle.fbx";
    std::string walkAnimPath = "assets/models/characters/animations/YBot_Walking.fbx";
    std::string jogAnimPath = "assets/models/characters/animations/YBot_JogForward.fbx";
    std::string walkBackAnimPath = "assets/models/characters/animations/YBot_Walking_Backward.fbx";
    
    // Strafe animations
    std::string strafeLeftAnimPath = "assets/models/characters/animations/YBot_Left_Strafe_Walking.fbx";
    std::string strafeRightAnimPath = "assets/models/characters/animations/YBot_Right_Strafe_Walking.fbx";
    std::string strafeForwardAnimPath = "assets/models/characters/animations/YBot_Rifle_Run.fbx";
    std::string strafeBackAnimPath = "assets/models/characters/animations/YBot_Running_Backward.fbx";
    
    // Aim animations
    std::string aimIdleAnimPath = "assets/models/characters/animations/YBot_Rifle_Aiming_Idle.fbx";
    
    // Visual transform
    glm::vec3 modelScale{0.01f};
    glm::vec3 modelRotation{0.0f, 180.0f, 0.0f};  // Y_Bot faces opposite direction
    glm::vec3 modelOffset{0.0f, -1.0f, 0.0f};
    
    // Blend space thresholds
    float walkThreshold = 0.5f;
    float runThreshold = 4.0f;
    
    // Transition durations
    float modeTransitionDuration = 0.3f;
    float locomotionBlendDuration = 0.15f;
};

class AdvancedCharacterAnimator : public se::Component {
public:
    AdvancedCharacterAnimator() = default;
    ~AdvancedCharacterAnimator() override = default;
    
    void Awake() override;
    void Start() override;
    void Update(float dt) override;
    
    // Mode management
    LocomotionMode GetLocomotionMode() const { return currentMode_; }
    void SetLocomotionMode(LocomotionMode mode);
    
    // State queries
    float GetCurrentVelocity() const { return currentVelocity_; }
    glm::vec2 GetStrafeInput() const { return strafeInput_; }
    bool IsAiming() const { return currentMode_ == LocomotionMode::Aiming; }
    
    // Layer queries
    float GetAimLayerWeight() const { return aimLayerWeight_; }
    bool IsLookAtEnabled() const { return lookAtEnabled_; }
    glm::vec2 GetLookAtAngles() const;
    
    // Body rotation queries
    float GetBodyYaw() const;
    bool IsBodyRotating() const;
    
    // Configuration
    AdvancedCharacterAnimatorConfig& GetConfig() { return config_; }
    
private:
    bool LoadModel();
    void SetupAnimator();
    void SetupBlendSpaces();
    void SetupLayers();
    void SetupLookAt();
    void SetupBodyRotation();
    
    void ProcessInput();
    void UpdateLocomotion(float dt);
    void UpdateAiming(float dt);
    void UpdateLayers(float dt);
    void UpdateLookAt(float dt);
    void UpdateBodyRotation(float dt);
    void ApplyFinalPose();
    
    AdvancedCharacterAnimatorConfig config_;
    
    // Visual entity
    se::Entity visualEntity_;
    std::shared_ptr<se::SkinnedModelData> modelData_;
    se::AnimatorComponent* animatorComp_ = nullptr;
    
    // Mode
    LocomotionMode currentMode_ = LocomotionMode::Standing;
    LocomotionMode targetMode_ = LocomotionMode::Standing;
    float modeTransitionWeight_ = 0.0f;
    
    // Locomotion state
    float currentVelocity_ = 0.0f;
    float targetVelocity_ = 0.0f;
    glm::vec2 strafeInput_{0.0f};
    bool isMoving_ = false;
    
    // Animation time tracking
    float animationTime_ = 0.0f;
    
    // Blend spaces
    std::unique_ptr<se::anim::BlendSpace1D> locomotionBlendSpace_;
    std::unique_ptr<se::anim::BlendSpace2D> strafeBlendSpace_;
    
    // Layer stack
    se::anim::AnimationLayerStack layerStack_;
    float aimLayerWeight_ = 0.0f;
    float targetAimWeight_ = 0.0f;
    
    // Procedural controllers
    std::unique_ptr<se::anim::LookAtController> lookAtController_;
    std::unique_ptr<se::anim::BodyRotationController> bodyRotationController_;
    bool lookAtEnabled_ = true;
    
    // Poses
    se::anim::Pose basePose_;
    se::anim::Pose aimPose_;
    se::anim::Pose finalPose_;
    
    // Animation clips (cached)
    std::shared_ptr<se::AnimationClip> idleClip_;
    std::shared_ptr<se::AnimationClip> walkClip_;
    std::shared_ptr<se::AnimationClip> jogClip_;
    std::shared_ptr<se::AnimationClip> aimIdleClip_;
};

} // namespace AnimationTest
