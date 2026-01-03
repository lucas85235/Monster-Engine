#include "AdvancedCharacterAnimator.h"

#include "engine/ecs/SimpleComponents.h"
#include "engine/ecs/SkinnedModelComponent.h"
#include "engine/ecs/AnimatorComponent.h"
#include "engine/animation/AnimationManager.h"
#include "engine/animation/SkinnedModelManager.h"
#include "engine/gameplay/Character.h"
#include "engine/gameplay/PlayerController.h"
#include "engine/input/InputManager.h"
#include "engine/input/MouseCodes.h"

namespace AnimationTest {

void AdvancedCharacterAnimator::Awake() {
}

void AdvancedCharacterAnimator::Start() {
    if (!LoadModel()) {
        SE_LOG_ERROR("[AdvancedCharacterAnimator] Failed to load model");
        return;
    }
    
    // Configure Character's model yaw offset based on visual rotation
    // This ensures physics rotation matches visual appearance
    if (auto* character = GetEntity().FindComponent<se::Character>()) {
        character->GetMovementConfig().modelYawOffset = config_.modelRotation.y;
    }
    
    SetupAnimator();
    SetupBlendSpaces();
    SetupLayers();
    SetupLookAt();
    SetupBodyRotation();
    
    SE_LOG_INFO("[AdvancedCharacterAnimator] Initialized successfully");
}

void AdvancedCharacterAnimator::Update(float dt) {
    if (!modelData_ || !animatorComp_) {
        return;
    }
    
    ProcessInput();
    UpdateLocomotion(dt);
    UpdateAiming(dt);
    UpdateLayers(dt);
    UpdateLookAt(dt);
    UpdateBodyRotation(dt);
    ApplyFinalPose();
    
    // Update animation time
    animationTime_ += dt;
}

bool AdvancedCharacterAnimator::LoadModel() {
    auto skinnedModel = se::SkinnedModelManager::Load(config_.modelPath);
    if (!skinnedModel) {
        SE_LOG_ERROR("[AdvancedCharacterAnimator] Failed to load model: {}", config_.modelPath);
        return false;
    }
    
    // Create visual entity as child
    visualEntity_ = GetScene()->CreateEntity("CharacterModel");
    visualEntity_.SetParent(GetEntity());
    
    // Apply transform corrections
    auto& transform = visualEntity_.GetComponent<se::TransformComponent>();
    transform.SetScale(config_.modelScale);
    transform.SetRotation(config_.modelRotation);
    transform.SetPosition(config_.modelOffset);
    
    // Add skinned model component
    visualEntity_.AddComponent<se::SkinnedModelComponent>(skinnedModel);
    modelData_ = skinnedModel->GetModelData();
    
    // Initialize poses with bone count
    size_t boneCount = modelData_->Bones.size();
    basePose_.Resize(boneCount);
    aimPose_.Resize(boneCount);
    finalPose_.Resize(boneCount);
    
    SE_LOG_INFO("[AdvancedCharacterAnimator] Loaded model with {} bones", boneCount);
    return true;
}

void AdvancedCharacterAnimator::SetupAnimator() {
    // Add animator component to visual entity
    auto& animComp = visualEntity_.AddComponent<se::AnimatorComponent>();
    animComp.Init(modelData_);
    animatorComp_ = &animComp;
    
    // Load animation clips
    idleClip_ = se::AnimationManager::Load(config_.idleAnimPath);
    walkClip_ = se::AnimationManager::Load(config_.walkAnimPath);
    jogClip_ = se::AnimationManager::Load(config_.jogAnimPath);
    aimIdleClip_ = se::AnimationManager::Load(config_.aimIdleAnimPath);
    
    if (!idleClip_) {
        SE_LOG_WARN("[AdvancedCharacterAnimator] Failed to load idle animation");
    }
    if (!walkClip_) {
        SE_LOG_WARN("[AdvancedCharacterAnimator] Failed to load walk animation");
    }
    if (!jogClip_) {
        SE_LOG_WARN("[AdvancedCharacterAnimator] Failed to load jog animation");
    }
    
    // Start with idle
    if (idleClip_) {
        animComp.Play(idleClip_, true);
    }
}

void AdvancedCharacterAnimator::SetupBlendSpaces() {
    // 1D Locomotion blend space: Idle → Walk → Run based on velocity
    locomotionBlendSpace_ = std::make_unique<se::anim::BlendSpace1D>("Locomotion");
    locomotionBlendSpace_->SetBounds(0.0f, 8.0f);
    
    if (idleClip_) locomotionBlendSpace_->AddSample(idleClip_, 0.0f);
    if (walkClip_) locomotionBlendSpace_->AddSample(walkClip_, 2.0f);
    if (jogClip_) locomotionBlendSpace_->AddSample(jogClip_, 5.0f);
    
    // 2D Strafe blend space
    strafeBlendSpace_ = std::make_unique<se::anim::BlendSpace2D>("Strafe");
    strafeBlendSpace_->SetBounds(glm::vec2(-1.0f), glm::vec2(1.0f));
    
    // Load strafe animations
    auto strafeLeft = se::AnimationManager::Load(config_.strafeLeftAnimPath);
    auto strafeRight = se::AnimationManager::Load(config_.strafeRightAnimPath);
    auto strafeForward = se::AnimationManager::Load(config_.strafeForwardAnimPath);
    auto strafeBack = se::AnimationManager::Load(config_.strafeBackAnimPath);
    
    // Add samples at cardinal directions and center
    if (aimIdleClip_) strafeBlendSpace_->AddSample(aimIdleClip_, glm::vec2(0.0f, 0.0f));
    if (strafeForward) strafeBlendSpace_->AddSample(strafeForward, glm::vec2(0.0f, 1.0f));
    if (strafeBack) strafeBlendSpace_->AddSample(strafeBack, glm::vec2(0.0f, -1.0f));
    if (strafeLeft) strafeBlendSpace_->AddSample(strafeLeft, glm::vec2(-1.0f, 0.0f));
    if (strafeRight) strafeBlendSpace_->AddSample(strafeRight, glm::vec2(1.0f, 0.0f));
    
    SE_LOG_INFO("[AdvancedCharacterAnimator] Blend spaces configured");
}

void AdvancedCharacterAnimator::SetupLayers() {
    // Layer 0: Base Locomotion (full body)
    auto& baseLayer = layerStack_.AddLayer("BaseLocomotion", 0, se::anim::LayerBlendMode::Override);
    baseLayer.SetWeight(1.0f);
    auto fullBodyMask = se::anim::BoneMask::FullBody(modelData_.get());
    baseLayer.SetBoneMask(fullBodyMask);
    
    // Layer 1: Upper Body Aim (blend mode, upper body only)
    auto& aimLayer = layerStack_.AddLayer("UpperBodyAim", 1, se::anim::LayerBlendMode::Blend);
    aimLayer.SetWeight(0.0f);
    auto upperBodyMask = se::anim::BoneMask::UpperBody(modelData_.get());
    aimLayer.SetBoneMask(upperBodyMask);
    
    // Layer 2: Look At (additive, spine chain only)
    auto& lookAtLayer = layerStack_.AddLayer("LookAt", 2, se::anim::LayerBlendMode::Additive);
    lookAtLayer.SetWeight(1.0f);
    auto spineMask = se::anim::BoneMask::SpineChain(modelData_.get());
    lookAtLayer.SetBoneMask(spineMask);
    
    SE_LOG_INFO("[AdvancedCharacterAnimator] Animation layers configured ({} layers)", 
                layerStack_.GetLayerCount());
}

void AdvancedCharacterAnimator::SetupLookAt() {
    lookAtController_ = std::make_unique<se::anim::LookAtController>();
    
    auto settings = se::anim::LookAtSettings::DefaultMixamo();
    lookAtController_->Initialize(modelData_.get(), settings);
    
    SE_LOG_INFO("[AdvancedCharacterAnimator] Look-at controller initialized");
}

void AdvancedCharacterAnimator::SetupBodyRotation() {
    bodyRotationController_ = std::make_unique<se::anim::BodyRotationController>();
    
    auto settings = se::anim::BodyRotationSettings::Default();
    bodyRotationController_->Initialize(settings);
    
    // Set initial yaw from character transform
    auto& transform = GetEntity().GetComponent<se::TransformComponent>();
    bodyRotationController_->SetCurrentYaw(transform.Rotation.y);
    
    SE_LOG_INFO("[AdvancedCharacterAnimator] Body rotation controller initialized");
}

void AdvancedCharacterAnimator::ProcessInput() {
    auto& input = se::InputManager::Get();
    
    // Check for aim mode toggle (RMB)
    static bool wasRmbDown = false;
    bool isRmbDown = input.IsMouseButtonDown(se::Mouse::ButtonRight);
    
    if (isRmbDown && !wasRmbDown) {
        // Toggle aim mode
        if (currentMode_ == LocomotionMode::Standing) {
            SetLocomotionMode(LocomotionMode::Aiming);
        } else {
            SetLocomotionMode(LocomotionMode::Standing);
        }
    }
    wasRmbDown = isRmbDown;
    
    // Query Character component for movement state
    // Don't read keyboard directly - PlayerController/Character handles that
    if (auto* character = GetEntity().FindComponent<se::Character>()) {
        glm::vec3 velocity = character->GetVelocity();
        glm::vec2 horizontalVel(velocity.x, velocity.z);
        
        targetVelocity_ = glm::length(horizontalVel);
        isMoving_ = character->IsMoving();
        
        // Calculate strafe input based on velocity relative to character forward
        if (isMoving_) {
            auto& transform = GetEntity().GetComponent<se::TransformComponent>();
            glm::vec3 forward = transform.GetForward();
            glm::vec3 right = transform.GetRight();
            
            // Project velocity onto forward/right axes for strafe input
            strafeInput_.x = glm::dot(glm::vec3(velocity.x, 0, velocity.z), right);
            strafeInput_.y = glm::dot(glm::vec3(velocity.x, 0, velocity.z), forward);
            
            // Normalize
            float len = glm::length(strafeInput_);
            if (len > 0.1f) {
                strafeInput_ /= len;
            }
        } else {
            strafeInput_ = glm::vec2(0.0f);
        }
    }
}

void AdvancedCharacterAnimator::SetLocomotionMode(LocomotionMode mode) {
    if (mode == currentMode_) return;
    
    targetMode_ = mode;
    modeTransitionWeight_ = 0.0f;
    
    SE_LOG_INFO("[AdvancedCharacterAnimator] Switching to {} mode",
                mode == LocomotionMode::Aiming ? "Aiming" : "Standing");
}

void AdvancedCharacterAnimator::UpdateLocomotion(float dt) {
    // Smooth velocity
    float velocityLerpSpeed = 10.0f;
    currentVelocity_ = glm::mix(currentVelocity_, targetVelocity_, 1.0f - std::exp(-velocityLerpSpeed * dt));
    
    // Control Character's orientToMovement and rotation based on locomotion mode
    if (auto* character = GetEntity().FindComponent<se::Character>()) {
        if (currentMode_ == LocomotionMode::Standing) {
            // Standing mode: character rotates to face movement direction
            character->GetMovementConfig().orientToMovement = true;
        } else {
            // Aiming mode: character faces camera direction (PlayerController's yaw)
            character->GetMovementConfig().orientToMovement = false;
            
            // Get camera yaw from PlayerController and make character face that direction
            // Add 180° because SetTargetRotation uses movement-based offset, but for aiming
            // we want to face where camera is looking (opposite of movement offset)
            if (auto* controller = GetEntity().FindComponent<se::PlayerController>()) {
                float cameraYaw = controller->GetCurrentYaw() + 180.0f;
                character->SetTargetRotation(cameraYaw);
            }
        }
    }
    
    // Select animation based on velocity
    // This applies in both modes - legs should always animate based on movement
    if (animatorComp_) {
        std::shared_ptr<se::AnimationClip> targetClip = idleClip_;
        
        if (currentVelocity_ > config_.runThreshold) {
            targetClip = jogClip_;  // Running
        } else if (currentVelocity_ > config_.walkThreshold) {
            targetClip = walkClip_;  // Walking
        }
        
        // Crossfade to new clip if different (smooth transition)
        if (targetClip && animatorComp_->currentClip != targetClip) {
            animatorComp_->CrossfadeTo(targetClip, config_.locomotionBlendDuration, true);
        }
    }
}

void AdvancedCharacterAnimator::UpdateAiming(float dt) {
    // Transition mode weight
    if (targetMode_ != currentMode_) {
        float transitionSpeed = 1.0f / config_.modeTransitionDuration;
        modeTransitionWeight_ += transitionSpeed * dt;
        
        if (modeTransitionWeight_ >= 1.0f) {
            modeTransitionWeight_ = 1.0f;
            currentMode_ = targetMode_;
        }
    }
    
    // Update target aim weight based on mode
    if (currentMode_ == LocomotionMode::Aiming || targetMode_ == LocomotionMode::Aiming) {
        targetAimWeight_ = (currentMode_ == LocomotionMode::Aiming) ? 1.0f : 0.0f;
        
        // If transitioning, use transition weight
        if (targetMode_ != currentMode_) {
            if (targetMode_ == LocomotionMode::Aiming) {
                targetAimWeight_ = modeTransitionWeight_;
            } else {
                targetAimWeight_ = 1.0f - modeTransitionWeight_;
            }
        }
    } else {
        targetAimWeight_ = 0.0f;
    }
    
    // Smooth aim layer weight
    float aimLerpSpeed = 8.0f;
    aimLayerWeight_ = glm::mix(aimLayerWeight_, targetAimWeight_, 1.0f - std::exp(-aimLerpSpeed * dt));
    
    // NOTE: Locomotion animations are handled by UpdateLocomotion in both modes.
    // Upper body aiming overlay will be implemented when animation layer blending
    // is integrated with the Animator system.
}

void AdvancedCharacterAnimator::UpdateLayers(float dt) {
    (void)dt;
    
    // Sample current animation pose from Animator into basePose_
    if (animatorComp_ && animatorComp_->animator) {
        animatorComp_->animator->SampleCurrentPose(basePose_);
    }
    
    // Update layer weights
    layerStack_.SetLayerWeight("UpperBodyAim", aimLayerWeight_);
    
    // Set base layer pose
    layerStack_.SetLayerPose("BaseLocomotion", basePose_);
    
    // If aiming, sample aim pose for upper body
    if (aimLayerWeight_ > 0.01f && aimIdleClip_) {
        aimPose_.SetFromClip(aimIdleClip_.get(), animationTime_, modelData_.get());
        layerStack_.SetLayerPose("UpperBodyAim", aimPose_);
    }
}

void AdvancedCharacterAnimator::UpdateLookAt(float dt) {
    if (!lookAtController_ || !lookAtEnabled_) {
        return;
    }
    
    // Get character forward
    auto& transform = GetEntity().GetComponent<se::TransformComponent>();
    glm::vec3 forward = transform.GetForward();
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
    
    // In aim mode, look toward camera direction
    if (currentMode_ == LocomotionMode::Aiming) {
        // Get camera from player controller if available
        if (auto* controller = GetEntity().FindComponent<se::PlayerController>()) {
            // For now, just look forward
            // TODO: Get actual camera direction from PlayerController
            lookAtController_->SetTarget(forward, forward, up);
        }
    } else {
        // In standing mode, smoothly return to center
        lookAtController_->SetTargetAngles(0.0f, 0.0f);
    }
    
    lookAtController_->Update(dt);
}

void AdvancedCharacterAnimator::UpdateBodyRotation(float dt) {
    if (!bodyRotationController_) {
        return;
    }
    
    // In standing mode, the Character component handles body rotation via orientToMovement.
    // We just track the state for animation purposes (e.g., additive procedural spine rotations).
    // Don't apply rotation to transform here - that would conflict with Character::ApplyRotation.
    
    auto& transform = GetEntity().GetComponent<se::TransformComponent>();
    float currentYaw = transform.Rotation.y;
    
    // Update the controller state (for use in procedural animations)
    // but DON'T apply the result to transform - Character handles that
    bodyRotationController_->SetCurrentYaw(currentYaw);
}

void AdvancedCharacterAnimator::ApplyFinalPose() {
    if (!animatorComp_ || !animatorComp_->animator) {
        return;
    }
    
    // Start with base pose
    finalPose_ = basePose_;
    
    // Evaluate layer stack (applies upper body aim, look-at layer, etc.)
    layerStack_.Evaluate(finalPose_);
    
    // Apply procedural look-at adjustments
    if (lookAtController_ && lookAtEnabled_) {
        lookAtController_->ApplyToPose(finalPose_);
    }
    
    // Apply final pose to animator's bone matrices
    animatorComp_->animator->ApplyPose(finalPose_);
}

glm::vec2 AdvancedCharacterAnimator::GetLookAtAngles() const {
    if (lookAtController_) {
        return lookAtController_->GetCurrentAngles();
    }
    return glm::vec2(0.0f);
}

float AdvancedCharacterAnimator::GetBodyYaw() const {
    if (bodyRotationController_) {
        return bodyRotationController_->GetCurrentYaw();
    }
    return 0.0f;
}

bool AdvancedCharacterAnimator::IsBodyRotating() const {
    if (bodyRotationController_) {
        return bodyRotationController_->IsRotating();
    }
    return false;
}

} // namespace AnimationTest
