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
#include "engine/ecs/Entity.h"

#include <glm.hpp>
#include <memory>
#include <vector>

namespace se {
class SkinnedModelData;
class AnimatorComponent;
class SkinnedModel;
}

namespace AnimationTest {

enum class LocomotionMode {
    Standing,   // Normal movement with full body rotation
    Aiming      // Strafe movement with locked forward direction
};

struct AdvancedCharacterAnimatorConfig {
    std::string modelPath = "assets/models/characters/ue_mannequin.FBX";
    
    // Animation paths
    std::string idleAnimPath = "assets/models/characters/new_animations/M_Relaxed_Stand_Idle_Loop.fbx";
    std::string walkAnimPath = "assets/models/characters/new_animations/M_Neutral_Walk_Loop_F.fbx";
    std::string jogAnimPath = "assets/models/characters/new_animations/M_Neutral_Run_Loop_F.fbx";
    std::string walkBackAnimPath = "assets/models/characters/new_animations/M_Neutral_Walk_Loop_B.fbx";
    
    // Strafe animations (LL = left, RR = right)
    std::string strafeLeftAnimPath = "assets/models/characters/new_animations/walk_aim_rifle/MF_Rifle_Walk_Left.FBX";
    std::string strafeRightAnimPath = "assets/models/characters/new_animations/walk_aim_rifle/MF_Rifle_Walk_Right.FBX";
    std::string strafeForwardAnimPath = "assets/models/characters/new_animations/walk_aim_rifle/MF_Rifle_Walk_Fwd.FBX";
    std::string strafeBackAnimPath = "assets/models/characters/new_animations/walk_aim_rifle/MF_Rifle_Walk_Bwd.FBX";
    
    // Aim animations
    std::string aimIdleAnimPath = "assets/models/characters/new_animations/aim_offset_animations_neutral/M_Neutral_AO_Stand_X0_Y0.fbx";
    
    // Visual transform
    glm::vec3 modelScale{0.01f};
    glm::vec3 modelRotation{-180.0f, 180.0f, 0.0f};  // Y_Bot faces opposite direction
    glm::vec3 modelOffset{0.0f, -0.8f, 0.0f};
    
    // Blend space thresholds
    float walkThreshold = 0.5f;
    float runThreshold = 4.0f;
    
    // Movement speeds (used to restore after aiming)
    float normalWalkSpeed = 5.0f;
    float normalRunSpeed = 8.0f;
    
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
    
    // Aim offset queries for debug
    glm::vec2 GetAimOffsetValues() const { return currentAimOffset_; }
    const se::anim::BlendSpace2D* GetAimOffsetBlendSpace() const { return aimOffsetBlendSpace_.get(); }
    
    // ImGui debug rendering
    void RenderImGuiDebug();
    
    // Configuration
    AdvancedCharacterAnimatorConfig& GetConfig() { return config_; }
    
private:
    bool LoadModel();
    void SetupAnimator();
    void SetupBlendSpaces();
    void SetupLayers();
    void SetupLookAt();
    void SetupBodyRotation();
    void SetupAimOffset();
    void SetupRifle();  // Rifle attachment setup
    
    void ProcessInput();
    void UpdateLocomotion(float dt);
    void UpdateAiming(float dt);
    void UpdateLayers(float dt);
    void UpdateLookAt(float dt);
    void UpdateBodyRotation(float dt);
    void ApplyFinalPose();
    void ApplyProceduralUpperBodyLookAt();
    void UpdateRifleAttachment();  // Update rifle position to follow hand bone
    void ApplyLeftHandIK();        // IK for left hand to reach foregrip
    void DebugDrawBones();
    
    AdvancedCharacterAnimatorConfig config_;
    
    // Visual entity
    se::Entity visualEntity_;
    std::shared_ptr<se::SkinnedModelData> modelData_;
    se::AnimatorComponent* animatorComp_ = nullptr;
    
    // Mode
    LocomotionMode currentMode_ = LocomotionMode::Standing;
    LocomotionMode targetMode_ = LocomotionMode::Standing;
    float modeTransitionWeight_ = 0.0f;
    
    // Aim mode body rotation tracking
    float aimBaseYaw_ = 0.0f;        // Reference yaw for calculating aim offset threshold
    bool aimBaseYawInitialized_ = false;
    bool isAimBodyRotating_ = false;  // Hysteresis flag for smooth rotation
    
    // Locomotion state
    float currentVelocity_ = 0.0f;
    float targetVelocity_ = 0.0f;
    glm::vec2 strafeInput_{0.0f};
    bool isMoving_ = false;
    bool walkToggle_ = false;  // Alt toggles walk mode
    
    // Animation time tracking
    float animationTime_ = 0.0f;
    
    // Blend spaces
    std::unique_ptr<se::anim::BlendSpace1D> locomotionBlendSpace_;
    std::unique_ptr<se::anim::BlendSpace2D> strafeBlendSpace_;
    std::unique_ptr<se::anim::BlendSpace2D> aimOffsetBlendSpace_;  // New: aim offset
    
    // Layer stack
    se::anim::AnimationLayerStack layerStack_;
    float aimLayerWeight_ = 0.0f;
    float targetAimWeight_ = 0.0f;
    
    // Procedural controllers
    std::unique_ptr<se::anim::LookAtController> lookAtController_;
    std::unique_ptr<se::anim::BodyRotationController> bodyRotationController_;
    bool lookAtEnabled_ = true;
    bool debugDrawBones_ = false;  // Toggle with F8
    
    // Current aim offset values for debug
    glm::vec2 currentAimOffset_{0.0f};
    
    // Poses
    se::anim::Pose basePose_;
    se::anim::Pose aimPose_;
    se::anim::Pose strafePose_;  // Output from 2D strafe blend space
    se::anim::Pose aimOffsetPose_;  // Output from aim offset blend space
    se::anim::Pose finalPose_;
    
    // Animation clips (cached)
    std::shared_ptr<se::AnimationClip> idleClip_;
    std::shared_ptr<se::AnimationClip> walkClip_;
    std::shared_ptr<se::AnimationClip> jogClip_;
    std::shared_ptr<se::AnimationClip> aimIdleClip_;
    
    // Debug bone entities (for F8 visualization)
    std::vector<se::Entity> boneDebugEntities_;
    bool boneDebugEntitiesCreated_ = false;
    
    // Rifle attachment system
    se::Entity rifleEntity_;
    std::shared_ptr<se::SkinnedModel> rifleModel_;
    bool rifleVisible_ = false;
    float rifleVisibilityWeight_ = 0.0f;  // For smooth transition
    
    // Rifle IK configuration (offsets from rifle origin)
    glm::vec3 rifleOffset_{0.0f, 0.02f, -0.05f};         // Position offset from right hand bone
    glm::vec3 rifleRotation_{90.0f, 0.0f, 0.0f};         // Rotation offset (degrees)
    glm::vec3 rifleLeftHandTarget_{0.0f, 0.02f, 0.35f};  // Foregrip position relative to rifle
};

} // namespace AnimationTest
