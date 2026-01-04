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
#include "engine/input/KeyCodes.h"
#include "engine/input/GamepadCodes.h"
#include "engine/debug/DebugRenderer.h"
#include "engine/ui/native/world/WorldSpaceUIComponent.h"
#include "engine/ui/native/world/WorldSpaceUIElement.h"

#include <gtx/matrix_decompose.hpp>


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
    SetupAimOffset();
    SetupLayers();
    SetupLookAt();
    SetupBodyRotation();
    SetupRifle();
    
    // Log all bones for debugging (one time at start)
    if (modelData_) {
        SE_LOG_INFO("[AdvancedCharacterAnimator] === BONE LIST ({} bones) ===", modelData_->Bones.size());
        for (size_t i = 0; i < modelData_->Bones.size(); ++i) {
            const auto& bone = modelData_->Bones[i];
            SE_LOG_INFO("  [{}] {} (parent: {})", i, bone.Name, bone.ParentIndex);
        }
        SE_LOG_INFO("[AdvancedCharacterAnimator] === END BONE LIST ===");
    }
    
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
    
    // Update rifle attachment (position follows right hand bone)
    UpdateRifleAttachment();
    
    // Apply procedural look-at for upper body in aim mode
    if (currentMode_ == LocomotionMode::Aiming) {
        ApplyProceduralUpperBodyLookAt();
    }
    
    // Debug bone visualization
    if (debugDrawBones_) {
        DebugDrawBones();
    }
    
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
    strafePose_.Resize(boneCount);
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
    
    // 2D Strafe blend space - cross pattern for 4 cardinal directions + center
    // Since we only have Fwd, Bwd, Left, Right, diagonals will be interpolated
    strafeBlendSpace_ = std::make_unique<se::anim::BlendSpace2D>("RifleStrafe");
    strafeBlendSpace_->SetBounds(glm::vec2(-1.0f), glm::vec2(1.0f));
    
    const std::string rifleWalkPath = "assets/models/characters/new_animations/walk_aim_rifle/";
    
    // Load rifle walk animations for cardinal directions
    auto strafeF = se::AnimationManager::Load(rifleWalkPath + "MF_Rifle_Walk_Fwd.FBX");
    auto strafeB = se::AnimationManager::Load(rifleWalkPath + "MF_Rifle_Walk_Bwd.FBX");
    auto strafeL = se::AnimationManager::Load(rifleWalkPath + "MF_Rifle_Walk_Left.FBX");
    auto strafeR = se::AnimationManager::Load(rifleWalkPath + "MF_Rifle_Walk_Right.FBX");
    
    // Load aim idle for center (standing still while aiming)
    auto aimIdle = se::AnimationManager::Load("assets/models/characters/new_animations/aim_offset_animations_rifle/MM_Rifle_Idle_ADS_AO_CC.FBX");
    
    // Add samples in cross pattern:
    //       (0,1)        = Forward
    // (-1,0) (0,0) (1,0) = Left, Idle, Right
    //       (0,-1)       = Backward
    
    // Center (aim idle)
    if (aimIdle) {
        strafeBlendSpace_->AddSample(aimIdle, glm::vec2(0.0f, 0.0f));
        SE_LOG_INFO("[AdvancedCharacterAnimator] Added aim idle at center");
    } else if (idleClip_) {
        strafeBlendSpace_->AddSample(idleClip_, glm::vec2(0.0f, 0.0f));
        SE_LOG_WARN("[AdvancedCharacterAnimator] Using fallback idle for strafe center");
    }
    
    // Cardinal directions
    if (strafeF) {
        strafeBlendSpace_->AddSample(strafeF, glm::vec2(0.0f, 1.0f));
        SE_LOG_INFO("[AdvancedCharacterAnimator] Added rifle walk forward");
    }
    if (strafeB) {
        strafeBlendSpace_->AddSample(strafeB, glm::vec2(0.0f, -1.0f));
        SE_LOG_INFO("[AdvancedCharacterAnimator] Added rifle walk backward");
    }
    if (strafeL) {
        strafeBlendSpace_->AddSample(strafeL, glm::vec2(-1.0f, 0.0f));
        SE_LOG_INFO("[AdvancedCharacterAnimator] Added rifle walk left");
    }
    if (strafeR) {
        strafeBlendSpace_->AddSample(strafeR, glm::vec2(1.0f, 0.0f));
        SE_LOG_INFO("[AdvancedCharacterAnimator] Added rifle walk right");
    }
    
    SE_LOG_INFO("[AdvancedCharacterAnimator] Rifle strafe blend space configured (cross pattern, {} samples)", 
                strafeBlendSpace_->GetSampleCount());
}

void AdvancedCharacterAnimator::SetupAimOffset() {
    // Rifle Aim Offset BlendSpace2D - 5x3 grid
    // X axis = Yaw (horizontal aim): -135 (LB) to +135 (RB) degrees
    // Y axis = Pitch (vertical aim): -45 (Down) to +45 (Up) degrees
    aimOffsetBlendSpace_ = std::make_unique<se::anim::BlendSpace2D>("RifleAimOffset");
    aimOffsetBlendSpace_->SetBounds(glm::vec2(-135.0f, -45.0f), glm::vec2(135.0f, 45.0f));
    
    const std::string basePath = "assets/models/characters/new_animations/aim_offset_animations_rifle/";
    
    // Define the grid mapping:
    // Yaw values: LB=-135, L=-90, C=0, R=+90, RB=+135
    // Pitch values: D=-45, C=0, U=+45
    
    struct AimOffsetEntry {
        const char* suffix;  // File suffix (e.g., "CC", "LU")
        float yaw;
        float pitch;
    };
    
    const AimOffsetEntry entries[] = {
        // Center column (yaw = 0)
        {"CC", 0.0f, 0.0f},    // Center-Center
        {"CU", 0.0f, 45.0f},   // Center-Up
        {"CD", 0.0f, -45.0f},  // Center-Down
        
        // Left column (yaw = -90)
        {"LC", -90.0f, 0.0f},    // Left-Center
        {"LU", -90.0f, 45.0f},   // Left-Up
        {"LD", -90.0f, -45.0f},  // Left-Down
        
        // Right column (yaw = +90)
        {"RC", 90.0f, 0.0f},    // Right-Center
        {"RU", 90.0f, 45.0f},   // Right-Up
        {"RD", 90.0f, -45.0f},  // Right-Down
        
        // Left-Back column (yaw = -135)
        {"LBC", -135.0f, 0.0f},    // LeftBack-Center
        {"LBU", -135.0f, 45.0f},   // LeftBack-Up
        {"LBD", -135.0f, -45.0f},  // LeftBack-Down
        
        // Right-Back column (yaw = +135)
        {"RBC", 135.0f, 0.0f},    // RightBack-Center
        {"RBU", 135.0f, 45.0f},   // RightBack-Up
        {"RBD", 135.0f, -45.0f},  // RightBack-Down
    };
    
    int loadedCount = 0;
    
    for (const auto& entry : entries) {
        // Build filename: MM_Rifle_Idle_ADS_AO_{suffix}.FBX
        std::string filename = "MM_Rifle_Idle_ADS_AO_" + std::string(entry.suffix) + ".FBX";
        std::string fullPath = basePath + filename;
        
        auto clip = se::AnimationManager::Load(fullPath);
        
        if (clip) {
            aimOffsetBlendSpace_->AddSample(clip, glm::vec2(entry.yaw, entry.pitch));
            loadedCount++;
            SE_LOG_INFO("[AdvancedCharacterAnimator] Loaded aim offset: {} at ({}, {})", 
                       entry.suffix, entry.yaw, entry.pitch);
        } else {
            SE_LOG_WARN("[AdvancedCharacterAnimator] Failed to load aim offset: {}", filename);
        }
    }
    
    SE_LOG_INFO("[AdvancedCharacterAnimator] Rifle aim offset blendspace configured: {}/15 animations loaded", loadedCount);
}

void AdvancedCharacterAnimator::SetupLayers() {
    // Layer 0: Base Locomotion (full body)
    auto& baseLayer = layerStack_.AddLayer("BaseLocomotion", 0, se::anim::LayerBlendMode::Override);
    baseLayer.SetWeight(1.0f);
    auto fullBodyMask = se::anim::BoneMask::FullBody(modelData_.get());
    baseLayer.SetBoneMask(fullBodyMask);
    
    // Layer 1: Upper Body Aim (OVERRIDE mode - completely replaces upper body with aim pose)
    // Use Spine1 as root - this excludes Hips and Spine from the mask, keeping them in lower body
    // This prevents the strafe animation's root rotation from affecting aim
    auto& aimLayer = layerStack_.AddLayer("UpperBodyAim", 1, se::anim::LayerBlendMode::Override);
    aimLayer.SetWeight(0.0f);
    
    // Create upper body mask from Spine1 bone (excludes Hips and Spine)
    // Spine1 is above the root rotation bones, so aim pose won't be affected
    auto upperBodyMask = se::anim::BoneMask::FromBoneAndChildren(modelData_.get(), "mixamorig:Spine1");
    if (upperBodyMask.CountIncluded() == 0) {
        upperBodyMask = se::anim::BoneMask::FromBoneAndChildren(modelData_.get(), "Spine1");
    }
    if (upperBodyMask.CountIncluded() == 0) {
        // Final fallback to standard upper body
        upperBodyMask = se::anim::BoneMask::UpperBody(modelData_.get());
    }
    
    SE_LOG_INFO("[AdvancedCharacterAnimator] Upper body mask includes {} bones", upperBodyMask.CountIncluded());
    aimLayer.SetBoneMask(upperBodyMask);
    
    // Layer 2: Look At (additive, spine chain only) - disabled in aim mode
    auto& lookAtLayer = layerStack_.AddLayer("LookAt", 2, se::anim::LayerBlendMode::Additive);
    lookAtLayer.SetWeight(0.0f);  // Start disabled
    auto spineMask = se::anim::BoneMask::SpineChain(modelData_.get());
    lookAtLayer.SetBoneMask(spineMask);
    
    SE_LOG_INFO("[AdvancedCharacterAnimator] Animation layers configured ({} layers)", 
                layerStack_.GetLayerCount());
}

void AdvancedCharacterAnimator::SetupLookAt() {
    lookAtController_ = std::make_unique<se::anim::LookAtController>();
    
    // Use UE Mannequin settings for aggressive spine control
    auto settings = se::anim::LookAtSettings::DefaultUEMannequin();
    lookAtController_->Initialize(modelData_.get(), settings);
    
    SE_LOG_INFO("[AdvancedCharacterAnimator] Look-at controller initialized with UE Mannequin settings");
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
    
    // Check for aim mode toggle (RMB or RB on gamepad)
    static bool wasRmbDown = false;
    static bool wasRbDown = false;
    bool isRmbDown = input.IsMouseButtonDown(se::Mouse::ButtonRight);
    bool isRbDown = input.IsGamepadButtonDown(se::Gamepad::RightBumper);
    
    // Toggle on RMB press
    if (isRmbDown && !wasRmbDown) {
        if (currentMode_ == LocomotionMode::Standing) {
            SetLocomotionMode(LocomotionMode::Aiming);
        } else {
            SetLocomotionMode(LocomotionMode::Standing);
        }
    }
    wasRmbDown = isRmbDown;
    
    // Toggle on RB press
    if (isRbDown && !wasRbDown) {
        if (currentMode_ == LocomotionMode::Standing) {
            SetLocomotionMode(LocomotionMode::Aiming);
        } else {
            SetLocomotionMode(LocomotionMode::Standing);
        }
    }
    wasRbDown = isRbDown;
    
    // F8 toggles bone debug visualization
    static bool wasF8Down = false;
    bool isF8Down = input.IsKeyDown(se::Key::F8);
    if (isF8Down && !wasF8Down) {
        debugDrawBones_ = !debugDrawBones_;
        SE_LOG_INFO("[AdvancedCharacterAnimator] Bone debug: {}", debugDrawBones_ ? "ON" : "OFF");
    }
    wasF8Down = isF8Down;
    
    // Alt toggles walk mode (walk vs run in normal movement)
    static bool wasAltDown = false;
    bool isAltDown = input.IsKeyDown(se::Key::LeftAlt) || input.IsKeyDown(se::Key::RightAlt);
    if (isAltDown && !wasAltDown) {
        walkToggle_ = !walkToggle_;
        SE_LOG_INFO("[AdvancedCharacterAnimator] Walk mode: {}", walkToggle_ ? "ON" : "OFF");
        
        // Update character movement speeds
        if (auto* character = GetEntity().FindComponent<se::Character>()) {
            if (walkToggle_) {
                // Walk mode: slower speeds
                character->GetMovementConfig().maxWalkSpeed = config_.normalWalkSpeed * 0.5f;
                character->GetMovementConfig().maxRunSpeed = config_.normalWalkSpeed;  // Cap at walk speed
            } else {
                // Normal mode: restore speeds
                character->GetMovementConfig().maxWalkSpeed = config_.normalWalkSpeed;
                character->GetMovementConfig().maxRunSpeed = config_.normalRunSpeed;
            }
        }
    }
    wasAltDown = isAltDown;
    
    // Query Character component for movement state
    // Don't read keyboard directly - PlayerController/Character handles that
    if (auto* character = GetEntity().FindComponent<se::Character>()) {
        glm::vec3 velocity = character->GetVelocity();
        glm::vec2 horizontalVel(velocity.x, velocity.z);
        
        targetVelocity_ = glm::length(horizontalVel);
        isMoving_ = character->IsMoving();
        
        // Calculate target strafe input based on velocity relative to character forward
        glm::vec2 targetStrafeInput = glm::vec2(0.0f);
        
        // Use a higher threshold to prevent blend space issues with tiny velocities
        const float VELOCITY_THRESHOLD = 0.3f;
        float horizontalVelMag = glm::length(horizontalVel);
        
        if (isMoving_ && horizontalVelMag > VELOCITY_THRESHOLD) {
            auto& transform = GetEntity().GetComponent<se::TransformComponent>();
            glm::vec3 forward = transform.GetForward();
            glm::vec3 right = transform.GetRight();
            
            // Ensure forward/right are valid (non-zero length)
            float forwardLen = glm::length(forward);
            float rightLen = glm::length(right);
            if (forwardLen > 0.001f && rightLen > 0.001f) {
                // Normalize direction vectors to ensure consistent dot products
                forward = glm::normalize(forward);
                right = glm::normalize(right);
                
                // Project velocity onto forward/right axes for strafe input
                glm::vec3 velDir = glm::normalize(glm::vec3(velocity.x, 0, velocity.z));
                targetStrafeInput.x = glm::dot(velDir, right);
                targetStrafeInput.y = glm::dot(velDir, forward);
                
                // Snap very small values to zero
                if (std::abs(targetStrafeInput.x) < 0.1f) targetStrafeInput.x = 0.0f;
                if (std::abs(targetStrafeInput.y) < 0.1f) targetStrafeInput.y = 0.0f;
                
                // Renormalize if needed
                float len = glm::length(targetStrafeInput);
                if (len > 0.01f && len < 0.99f) {
                    targetStrafeInput = glm::normalize(targetStrafeInput);
                } else if (len < 0.01f) {
                    targetStrafeInput = glm::vec2(0.0f);
                }
            }
        }
        
        // SMOOTH interpolation for blend space transitions
        // Use exponential smoothing for natural feel
        // Note: Using fixed delta (1/60) since ProcessInput doesn't receive dt
        float strafeLerpSpeed = 8.0f;  // Higher = faster transitions
        const float fixedDt = 1.0f / 60.0f;
        float t = 1.0f - std::exp(-strafeLerpSpeed * fixedDt);
        strafeInput_.x = strafeInput_.x + (targetStrafeInput.x - strafeInput_.x) * t;
        strafeInput_.y = strafeInput_.y + (targetStrafeInput.y - strafeInput_.y) * t;
        
        // Snap to zero if very close (for clean idle)
        if (glm::length(strafeInput_) < 0.05f) {
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
            
            // Reset aim state flags
            aimBaseYawInitialized_ = false;
            isAimBodyRotating_ = false;
            
            // Apply walk toggle speeds
            if (walkToggle_) {
                character->GetMovementConfig().maxWalkSpeed = config_.normalWalkSpeed * 0.5f;
                character->GetMovementConfig().maxRunSpeed = config_.normalWalkSpeed;
            } else {
                character->GetMovementConfig().maxWalkSpeed = config_.normalWalkSpeed;
                character->GetMovementConfig().maxRunSpeed = config_.normalRunSpeed;
            }
        } else {
            // Aiming mode: character ALWAYS faces camera direction
            character->GetMovementConfig().orientToMovement = false;
            
            // Reduce movement speed in aiming mode (50% of normal)
            float aimSpeedMultiplier = walkToggle_ ? 0.3f : 0.5f;
            character->GetMovementConfig().maxWalkSpeed = config_.normalWalkSpeed * aimSpeedMultiplier;
            character->GetMovementConfig().maxRunSpeed = config_.normalWalkSpeed * aimSpeedMultiplier;
            
            // Always rotate character to face camera direction
            if (auto* controller = GetEntity().FindComponent<se::PlayerController>()) {
                float cameraYaw = controller->GetCurrentYaw();
                
                // Calculate where the character should face to look at camera direction
                float modelOffset = character->GetMovementConfig().modelYawOffset;
                float targetCharacterYaw = cameraYaw + 180.0f + modelOffset;
                
                // Normalize target to -180 to 180
                while (targetCharacterYaw > 180.0f) targetCharacterYaw -= 360.0f;
                while (targetCharacterYaw < -180.0f) targetCharacterYaw += 360.0f;
                
                // Always set target rotation - character follows camera continuously
                character->SetTargetRotationRaw(targetCharacterYaw);
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
    
    // In aiming mode, combine:
    // - LOWER BODY (hips, legs): Strafe blend space for movement animation
    // - UPPER BODY (spine and above): Aim offset blend space for aiming direction
    if (currentMode_ == LocomotionMode::Aiming || aimLayerWeight_ > 0.01f) {
        // Get current camera aim angles for aim offset
        float aimYaw = 0.0f;
        float aimPitch = 0.0f;
        
        if (auto* controller = GetEntity().FindComponent<se::PlayerController>()) {
            // Get camera pitch directly
            aimPitch = controller->GetCurrentPitch();
            
            // Yaw is relative to character facing
            float cameraYaw = controller->GetCurrentYaw();
            float characterYaw = GetEntity().GetComponent<se::TransformComponent>().Rotation.y;
            float modelOffset = 0.0f;
            if (auto* character = GetEntity().FindComponent<se::Character>()) {
                modelOffset = character->GetMovementConfig().modelYawOffset;
            }
            
            // Calculate aim yaw relative to character (for aim offset blend)
            // Camera looks at cameraYaw, character faces characterYaw (adjusted by modelOffset)
            aimYaw = cameraYaw - (characterYaw - modelOffset - 180.0f);
            
            // Normalize
            while (aimYaw > 180.0f) aimYaw -= 360.0f;
            while (aimYaw < -180.0f) aimYaw += 360.0f;
        }
        
        // Update current aim offset values (for debug display)
        // Note: Invert pitch because aim offset animations use opposite convention
        currentAimOffset_ = glm::vec2(aimYaw, -aimPitch);
        
        // 1. Sample STRAFE animation for leg movement
        if (strafeBlendSpace_ && modelData_) {
            strafeBlendSpace_->Evaluate(strafeInput_, strafePose_, animationTime_, modelData_.get());
        }
        
        // 2. Sample AIM OFFSET animation for upper body
        // Use CENTER pose (yaw=0, pitch based on camera) for stable base
        // The IK will handle the rest of the camera-following
        if (aimOffsetBlendSpace_ && modelData_) {
            // Only use vertical (pitch) from camera, horizontal is handled by IK
            glm::vec2 aimForUpperBody = glm::vec2(0.0f, currentAimOffset_.y);  // Yaw=0, only pitch
            glm::vec2 clampedAim = glm::clamp(aimForUpperBody, 
                                               glm::vec2(-135.0f, -45.0f), 
                                               glm::vec2(135.0f, 45.0f));
            aimOffsetBlendSpace_->Evaluate(clampedAim, aimOffsetPose_, 0.0f, modelData_.get());
        }
        
        // 3. Combine: Lower body from strafe, upper body from aim offset
        // Upper body uses AIM OFFSET ONLY (no strafe influence) - always faces forward
        // Lower body uses STRAFE for walking animation
        
        // Strategy: 
        // - Start with strafe pose (for legs/hips)
        // - Replace upper body bones with aim offset pose
        // - Apply COUNTER-ROTATION to spine_01 to cancel pelvis rotation from strafe
        basePose_ = strafePose_;
        
        // UE Mannequin bone hierarchy:
        // 0: root, 1: pelvis, 2: spine_01, 3: spine_02, 4: spine_03
        // 5+: clavicle_l, upperarm_l... (arms and neck/head)
        // Legs: 61+: thigh_l, calf_l, foot_l, thigh_r, calf_r, foot_r...
        
        // Find pelvis and spine indices
        int pelvisIndex = -1;
        int spineStartIndex = -1;
        if (modelData_) {
            pelvisIndex = modelData_->GetBoneIndex("pelvis");
            if (pelvisIndex < 0) pelvisIndex = modelData_->GetBoneIndex("Hips");
            if (pelvisIndex < 0) pelvisIndex = 1;  // Fallback
            
            spineStartIndex = modelData_->GetBoneIndex("spine_01");
            if (spineStartIndex < 0) spineStartIndex = modelData_->GetBoneIndex("Spine");
            if (spineStartIndex < 0) spineStartIndex = 2;  // Fallback
        }
        
        const size_t UPPER_BODY_START = static_cast<size_t>(spineStartIndex);
        const size_t LEG_START = 61;    // First leg bone (thigh_l)
        
        // Apply aim offset to ENTIRE upper body (spine_01 and above)
        for (size_t i = UPPER_BODY_START; i < LEG_START && i < basePose_.GetBoneCount() && i < aimOffsetPose_.GetBoneCount(); ++i) {
            basePose_[i] = aimOffsetPose_[i];
        }
        
        // CRITICAL: Counter-rotate spine_01 to cancel pelvis rotation from strafe
        // The pelvis rotates with strafe animation, which would propagate to children
        // We need to cancel this by rotating spine_01 in the OPPOSITE direction
        if (pelvisIndex >= 0 && spineStartIndex >= 0 && 
            static_cast<size_t>(pelvisIndex) < strafePose_.GetBoneCount() &&
            static_cast<size_t>(spineStartIndex) < basePose_.GetBoneCount()) {
            
            // Get pelvis rotation from strafe animation
            glm::quat strafePelvisRot = strafePose_[static_cast<size_t>(pelvisIndex)].rotation;
            
            // Get pelvis rotation from aim offset (which should be neutral/forward)
            glm::quat aimPelvisRot = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);  // Identity (neutral)
            if (static_cast<size_t>(pelvisIndex) < aimOffsetPose_.GetBoneCount()) {
                aimPelvisRot = aimOffsetPose_[static_cast<size_t>(pelvisIndex)].rotation;
            }
            
            // Calculate the difference (how much pelvis rotated from strafe)
            glm::quat pelvisDelta = glm::inverse(aimPelvisRot) * strafePelvisRot;
            
            // Apply INVERSE of this delta to spine_01 to cancel the effect
            glm::quat counterRotation = glm::inverse(pelvisDelta);
            
            // Apply counter-rotation to spine_01
            auto& spine01 = basePose_[static_cast<size_t>(spineStartIndex)];
            spine01.rotation = glm::normalize(counterRotation * spine01.rotation);
        }
        
        // Legs (61+) have strafe pose
        // Root (0), pelvis (1) have strafe pose
    } else {
        // Standing mode: sample from Animator's crossfade output
        if (animatorComp_ && animatorComp_->animator) {
            animatorComp_->animator->SampleCurrentPose(basePose_);
        }
    }
    
    // Update layer weights
    layerStack_.SetLayerWeight("UpperBodyAim", aimLayerWeight_);
    
    // Set base layer pose
    layerStack_.SetLayerPose("BaseLocomotion", basePose_);
}

void AdvancedCharacterAnimator::UpdateLookAt(float dt) {
    if (!lookAtController_ || !lookAtEnabled_) {
        return;
    }
    
    // In aim mode, upper body should always face camera direction
    // The LookAt controller adjusts spine/neck/head to look at target
    if (currentMode_ == LocomotionMode::Aiming) {
        // In aiming mode, don't use look-at controller since upper body
        // rotation is handled by the aim pose. Keep centered.
        lookAtController_->SetTargetAngles(0.0f, 0.0f);
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
    
    // In standing mode with no aim layer, let the Animator's crossfade work naturally
    // Only override when we're actually doing layer blending
    if (aimLayerWeight_ < 0.01f && currentMode_ == LocomotionMode::Standing) {
        // Don't override - Animator handles crossfade on its own
        return;
    }
    
    // Start with base pose
    finalPose_ = basePose_;
    
    // Evaluate layer stack (applies upper body aim, look-at layer, etc.)
    layerStack_.Evaluate(finalPose_);
    
    // Apply PROCEDURAL SPINE IK as FINAL step (guarantees stable aiming)
    // This overrides any animation influence to ensure upper body always faces camera
    if (lookAtController_ && currentMode_ == LocomotionMode::Aiming) {
        if (auto* controller = GetEntity().FindComponent<se::PlayerController>()) {
            // Get camera pitch - negative pitch = looking down
            float camPitch = controller->GetCurrentPitch();
            
            // Invert pitch for bones (looking up = positive bone rotation)
            float bonePitch = -camPitch;
            
            // For yaw, we want the spine to compensate for any residual offset
            // The body should already be facing camera, so yaw should be minimal
            // But we apply it anyway for stability during quick movements
            float camYaw = controller->GetCurrentYaw();
            float charYaw = GetEntity().GetComponent<se::TransformComponent>().Rotation.y;
            float modelOffset = 0.0f;
            if (auto* character = GetEntity().FindComponent<se::Character>()) {
                modelOffset = character->GetMovementConfig().modelYawOffset;
            }
            
            // Calculate relative yaw (how much character deviates from camera)
            float relativeYaw = camYaw - (charYaw - modelOffset - 180.0f);
            while (relativeYaw > 180.0f) relativeYaw -= 360.0f;
            while (relativeYaw < -180.0f) relativeYaw += 360.0f;
            
            // Apply immediate procedural override - no smoothing needed, character is already facing camera
            // The IK will distribute rotation across spine_03, neck, head for stable aiming
            lookAtController_->ApplyImmediateOverride(finalPose_, relativeYaw, bonePitch);
        }
    } else if (lookAtController_ && lookAtEnabled_) {
        // Non-aim mode: use standard look-at (if enabled)
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

void AdvancedCharacterAnimator::DebugDrawBones() {
    if (!animatorComp_ || !animatorComp_->animator || !modelData_) {
        return;
    }
    
    // Create bone debug entities on first call
    if (!boneDebugEntitiesCreated_) {
        boneDebugEntitiesCreated_ = true;
        
        SE_LOG_INFO("[AdvancedCharacterAnimator] Creating bone debug entities for {} bones...", modelData_->Bones.size());
        
        for (size_t i = 0; i < modelData_->Bones.size(); ++i) {
            const auto& bone = modelData_->Bones[i];
            
            // Create entity for this bone
            se::Entity boneEntity = GetScene()->CreateEntity("Bone_" + bone.Name);
            
            // Add WorldSpaceUIComponent with bone name
            auto& worldUI = boneEntity.AddComponent<se::WorldSpaceUIComponent>();
            worldUI.offset = {0.0f, 0.05f, 0.0f};  // Slightly above bone position
            worldUI.baseScale = 0.8f;
            worldUI.scaleByDistance = true;
            worldUI.minScale = 0.3f;
            worldUI.maxScale = 1.5f;
            worldUI.depthTest = false;
            worldUI.referenceDistance = 5.0f;
            
            auto textLabel = worldUI.AddElement<se::WorldSpaceText>();
            textLabel->text = bone.Name;
            textLabel->color = {1.0f, 1.0f, 0.0f, 1.0f};  // Yellow
            textLabel->fontSize = 3.0f;
            textLabel->centered = true;
            
            boneDebugEntities_.push_back(boneEntity);
            
            SE_LOG_DEBUG("  Created entity for bone: {}", bone.Name);
        }
        
        SE_LOG_INFO("[AdvancedCharacterAnimator] Created {} bone debug entities", boneDebugEntities_.size());
    }
    // Update bone entity positions
    // Use Entity::ComputeWorldMatrix() to walk hierarchy correctly
    glm::mat4 worldMatrix = visualEntity_.ComputeWorldMatrix();
    
    // Debug: Verify hierarchy and rotation (log once)
    static bool hierarchyLogged = false;
    if (!hierarchyLogged) {
        hierarchyLogged = true;
        auto& visualTransform = visualEntity_.GetComponent<se::TransformComponent>();
        SE_LOG_INFO("=== DEBUG HIERARCHY ===");
        SE_LOG_INFO("visualEntity_.HasParent() = {}", visualEntity_.HasParent());
        if (visualEntity_.HasParent()) {
            auto parent = visualEntity_.GetParent();
            SE_LOG_INFO("Parent valid = {}", parent.IsValid());
        }
        SE_LOG_INFO("visualEntity_ Rotation = ({}, {}, {})", 
                    visualTransform.Rotation.x, visualTransform.Rotation.y, visualTransform.Rotation.z);
        SE_LOG_INFO("visualEntity_ GetTransform()[0] = ({}, {}, {}, {})", 
                    visualTransform.GetTransform()[0][0], visualTransform.GetTransform()[0][1],
                    visualTransform.GetTransform()[0][2], visualTransform.GetTransform()[0][3]);
        SE_LOG_INFO("ComputeWorldMatrix()[0] = ({}, {}, {}, {})",
                    worldMatrix[0][0], worldMatrix[0][1], worldMatrix[0][2], worldMatrix[0][3]);
        SE_LOG_INFO("=== END DEBUG ===");
    }
    
    const auto& boneMatrices = animatorComp_->animator->GetBoneMatrices();
    
    for (size_t i = 0; i < boneDebugEntities_.size() && i < boneMatrices.size(); ++i) {
        if (!boneDebugEntities_[i].IsValid()) continue;
        
        const auto& bone = modelData_->Bones[i];
        
        // Get bone world position
        glm::mat4 boneWorld = worldMatrix * glm::inverse(modelData_->GlobalInverseTransform) * boneMatrices[i] * glm::inverse(bone.OffsetMatrix);
        glm::vec3 bonePos = glm::vec3(boneWorld[3]);
        
        // Update entity position
        auto& boneTransform = boneDebugEntities_[i].GetComponent<se::TransformComponent>();
        boneTransform.SetPosition(bonePos);
        
        // Draw sphere for bone position
        se::DebugRenderer::Get().DrawSphere(bonePos, 0.02f, glm::vec3(0.0f, 1.0f, 0.0f));
    }
}

void AdvancedCharacterAnimator::ApplyProceduralUpperBodyLookAt() {
    if (!animatorComp_ || !animatorComp_->animator || !modelData_) {
        return;
    }
    
    // Check if aim offset blendspace is available
    if (!aimOffsetBlendSpace_) {
        return;
    }
    
    // Get camera direction from PlayerController
    auto* controller = GetEntity().FindComponent<se::PlayerController>();
    if (!controller) {
        return;
    }
    
    // Get camera yaw and pitch (where camera is looking)
    float cameraYaw = controller->GetCurrentYaw();
    float cameraPitch = controller->GetCurrentPitch();
    
    // Get character's current facing direction (from transform)
    auto& charTransform = GetEntity().GetComponent<se::TransformComponent>();
    float characterYaw = charTransform.Rotation.y;
    
    // Calculate the difference between camera yaw and character yaw
    // This is the aim offset yaw (how much the character is looking away from body direction)
    // Negate to match mouse movement direction
    float aimYaw = -(cameraYaw - characterYaw);
    
    // Normalize to -180 to 180
    while (aimYaw > 180.0f) aimYaw -= 360.0f;
    while (aimYaw < -180.0f) aimYaw += 360.0f;
    
    // Clamp to blendspace bounds (-135 to +135 for yaw, -90 to +90 for pitch)
    // Negate pitch to match mouse movement direction
    aimYaw = glm::clamp(aimYaw, -135.0f, 135.0f);
    float aimPitch = glm::clamp(-cameraPitch, -90.0f, 90.0f);
    
    // Sample the aim offset blendspace
    // BlendSpace2D expects (parameter, outPose, time, skeleton)
    aimOffsetPose_.Resize(modelData_->Bones.size());
    
    // Debug: log aim values once per second
    static float debugTimer = 0.0f;
    debugTimer += 0.016f;
    if (debugTimer > 1.0f) {
        SE_LOG_INFO("[AimOffset] cameraYaw={:.1f} charYaw={:.1f} aimYaw={:.1f} aimPitch={:.1f}", 
                    cameraYaw, characterYaw, aimYaw, aimPitch);
        debugTimer = 0.0f;
    }
    
    aimOffsetBlendSpace_->Evaluate(glm::vec2(aimYaw, aimPitch), aimOffsetPose_, animationTime_, modelData_.get());
    
    // Store for debug display
    currentAimOffset_ = glm::vec2(aimYaw, aimPitch);
    
    // Apply aim offset ONLY to neck and head bones
    // UE Mannequin skeleton: neck_01=58, neck_02=59, head=60
    const size_t NECK_01 = 58;
    const size_t NECK_02 = 59;
    const size_t HEAD = 60;
    
    // Debug: log blendspace sample count once
    static bool loggedSamples = false;
    if (!loggedSamples) {
        SE_LOG_INFO("[AimOffset] BlendSpace has {} samples, applying to neck/head only (bones 58-60)", 
                    aimOffsetBlendSpace_->GetSampleCount());
        loggedSamples = true;
    }
    
    // Only modify neck and head bones from aim offset
    if (NECK_01 < finalPose_.GetBoneCount() && NECK_01 < aimOffsetPose_.GetBoneCount()) {
        finalPose_[NECK_01] = aimOffsetPose_[NECK_01];
    }
    if (NECK_02 < finalPose_.GetBoneCount() && NECK_02 < aimOffsetPose_.GetBoneCount()) {
        finalPose_[NECK_02] = aimOffsetPose_[NECK_02];
    }
    if (HEAD < finalPose_.GetBoneCount() && HEAD < aimOffsetPose_.GetBoneCount()) {
        finalPose_[HEAD] = aimOffsetPose_[HEAD];
    }
    
    // Re-apply pose to animator
    animatorComp_->animator->ApplyPose(finalPose_);
}

void AdvancedCharacterAnimator::RenderImGuiDebug() {
    if (!ImGui::CollapsingHeader("Aim Offset Debug", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }
    
    // Display current aim values
    ImGui::Text("Aim Yaw: %.1f", currentAimOffset_.x);
    ImGui::Text("Aim Pitch: %.1f", currentAimOffset_.y);
    
    if (!aimOffsetBlendSpace_) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "No BlendSpace loaded");
        return;
    }
    
    ImGui::Text("Samples: %zu", aimOffsetBlendSpace_->GetSampleCount());
    ImGui::Text("Cached Triangle: %d", aimOffsetBlendSpace_->GetCachedTriangle());
    
    ImGui::Separator();
    
    // Draw 2D blendspace visualization
    ImVec2 canvasSize(300, 200);
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    // Background
    drawList->AddRectFilled(canvasPos, 
        ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), 
        IM_COL32(40, 40, 40, 255));
    
    // Border
    drawList->AddRect(canvasPos, 
        ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), 
        IM_COL32(100, 100, 100, 255));
    
    // Get bounds
    glm::vec2 minBounds = aimOffsetBlendSpace_->GetMinBounds();
    glm::vec2 maxBounds = aimOffsetBlendSpace_->GetMaxBounds();
    
    auto toCanvas = [&](glm::vec2 pos) -> ImVec2 {
        float nx = (pos.x - minBounds.x) / (maxBounds.x - minBounds.x);
        float ny = (pos.y - minBounds.y) / (maxBounds.y - minBounds.y);
        return ImVec2(
            canvasPos.x + nx * canvasSize.x,
            canvasPos.y + (1.0f - ny) * canvasSize.y  // Flip Y
        );
    };
    
    // Draw grid lines
    for (float x = minBounds.x; x <= maxBounds.x; x += 45.0f) {
        ImVec2 p1 = toCanvas(glm::vec2(x, minBounds.y));
        ImVec2 p2 = toCanvas(glm::vec2(x, maxBounds.y));
        drawList->AddLine(p1, p2, IM_COL32(60, 60, 60, 255));
    }
    for (float y = minBounds.y; y <= maxBounds.y; y += 45.0f) {
        ImVec2 p1 = toCanvas(glm::vec2(minBounds.x, y));
        ImVec2 p2 = toCanvas(glm::vec2(maxBounds.x, y));
        drawList->AddLine(p1, p2, IM_COL32(60, 60, 60, 255));
    }
    
    // Draw triangles
    const auto& triangles = aimOffsetBlendSpace_->GetTriangles();
    const auto& samples = aimOffsetBlendSpace_->GetSamples();
    int cachedTriangle = aimOffsetBlendSpace_->GetCachedTriangle();
    
    for (size_t i = 0; i < triangles.size(); ++i) {
        const auto& tri = triangles[i];
        if (tri[0] < 0 || tri[1] < 0 || tri[2] < 0) continue;
        if (static_cast<size_t>(tri[0]) >= samples.size() || 
            static_cast<size_t>(tri[1]) >= samples.size() || 
            static_cast<size_t>(tri[2]) >= samples.size()) continue;
            
        ImVec2 p0 = toCanvas(samples[tri[0]].position);
        ImVec2 p1 = toCanvas(samples[tri[1]].position);
        ImVec2 p2 = toCanvas(samples[tri[2]].position);
        
        ImU32 lineColor = (static_cast<int>(i) == cachedTriangle) 
            ? IM_COL32(255, 200, 50, 255)   // Highlight active triangle
            : IM_COL32(100, 100, 100, 255);
        
        drawList->AddLine(p0, p1, lineColor);
        drawList->AddLine(p1, p2, lineColor);
        drawList->AddLine(p2, p0, lineColor);
    }
    
    // Draw sample points
    for (const auto& sample : samples) {
        ImVec2 p = toCanvas(sample.position);
        drawList->AddCircleFilled(p, 4.0f, IM_COL32(150, 150, 150, 255));
    }
    
    // Draw current position (green X)
    ImVec2 currentPos = toCanvas(currentAimOffset_);
    float crossSize = 8.0f;
    drawList->AddLine(
        ImVec2(currentPos.x - crossSize, currentPos.y - crossSize),
        ImVec2(currentPos.x + crossSize, currentPos.y + crossSize),
        IM_COL32(0, 255, 0, 255), 2.0f);
    drawList->AddLine(
        ImVec2(currentPos.x - crossSize, currentPos.y + crossSize),
        ImVec2(currentPos.x + crossSize, currentPos.y - crossSize),
        IM_COL32(0, 255, 0, 255), 2.0f);
    
    // Reserve space for canvas
    ImGui::Dummy(canvasSize);
    
    // Axis labels
    ImGui::Text("X: Yaw (%.0f to %.0f)  Y: Pitch (%.0f to %.0f)", 
                minBounds.x, maxBounds.x, minBounds.y, maxBounds.y);
}

void AdvancedCharacterAnimator::SetupRifle() {
    // Load rifle model
    rifleModel_ = se::SkinnedModelManager::Load("assets/models/weapons/place_holder_rifle.fbx");
    if (!rifleModel_) {
        SE_LOG_WARN("[AdvancedCharacterAnimator] Failed to load rifle model");
        return;
    }
    
    // Create rifle entity (NOT as child - we'll set world position directly)
    rifleEntity_ = GetScene()->CreateEntity("Rifle");
    
    // Add skinned model component for the rifle
    auto& skinnedComp = rifleEntity_.AddComponent<se::SkinnedModelComponent>(rifleModel_);
    
    // Initial visibility - hidden until aim mode
    skinnedComp.IsVisible = false;
    
    SE_LOG_INFO("[AdvancedCharacterAnimator] Rifle loaded and attached");
}

void AdvancedCharacterAnimator::UpdateRifleAttachment() {
    if (!rifleEntity_.IsValid() || !animatorComp_ || !animatorComp_->animator) {
        return;
    }
    
    // Target visibility based on aim mode
    bool targetVisible = (currentMode_ == LocomotionMode::Aiming);
    
    // Smooth visibility transition
    float transitionSpeed = 10.0f;
    float dt = 1.0f / 60.0f;  // Approximate, could use actual dt
    if (targetVisible) {
        rifleVisibilityWeight_ = glm::min(rifleVisibilityWeight_ + transitionSpeed * dt, 1.0f);
    } else {
        rifleVisibilityWeight_ = glm::max(rifleVisibilityWeight_ - transitionSpeed * dt, 0.0f);
    }
    
    // Update visibility state
    bool shouldBeVisible = rifleVisibilityWeight_ > 0.01f;
    if (shouldBeVisible != rifleVisible_) {
        rifleVisible_ = shouldBeVisible;
        if (rifleEntity_.HasComponent<se::SkinnedModelComponent>()) {
            rifleEntity_.GetComponent<se::SkinnedModelComponent>().IsVisible = rifleVisible_;
        }
    }
    
    if (!rifleVisible_) {
        return;
    }
    
    // Get right hand bone world matrix
    // UE Mannequin skeleton uses "hand_r" naming convention
    int handBoneIndex = modelData_->GetBoneIndex("hand_r");
    if (handBoneIndex < 0) {
        handBoneIndex = modelData_->GetBoneIndex("RightHand");
    }
    if (handBoneIndex < 0) {
        handBoneIndex = modelData_->GetBoneIndex("mixamorig:RightHand");
    }
    
    if (handBoneIndex < 0) {
        // Log once and return
        static bool loggedOnce = false;
        if (!loggedOnce) {
            SE_LOG_WARN("[AdvancedCharacterAnimator] Could not find right hand bone in skeleton");
            loggedOnce = true;
        }
        return;
    }
    glm::mat4 rightHandBone = animatorComp_->animator->GetBoneWorldMatrix(handBoneIndex);
    
    // Use the new Entity::ComputeWorldMatrix() which walks up the parent hierarchy
    // to compute the correct world matrix including all parent transforms
    glm::mat4 visualWorld = visualEntity_.ComputeWorldMatrix();
    
    // Transform bone matrix to world space: WorldMatrix * bonePose
    glm::mat4 handWorld = visualWorld * rightHandBone;
    
    // Extract world position from hand bone
    glm::vec3 handWorldPos = glm::vec3(handWorld[3]);
    
    // Extract world rotation from hand bone
    glm::vec3 scale, translation, skew;
    glm::quat handWorldRot;
    glm::vec4 perspective;
    glm::decompose(handWorld, scale, handWorldRot, translation, skew, perspective);
    
    // Apply rifle offset in hand's local space (rotated by hand's world rotation)
    glm::vec3 rifleWorldPos = handWorldPos + handWorldRot * rifleOffset_;
    
    // Calculate rifle world rotation: hand world rotation + rifle orientation offset
    glm::quat rifleRotOffset = glm::quat(glm::radians(rifleRotation_));
    glm::quat rifleWorldRot = handWorldRot * rifleRotOffset;
    
    // Set rifle WORLD transform directly (no parenting)
    auto& rifleTransform = rifleEntity_.GetComponent<se::TransformComponent>();
    rifleTransform.SetPosition(rifleWorldPos);
    rifleTransform.SetRotation(glm::degrees(glm::eulerAngles(rifleWorldRot)));
    // Scale needs to match the model scale since we're not parented
    rifleTransform.SetScale(config_.modelScale);
    
    // Debug visualization: draw spheres at hand and rifle positions
    if (debugDrawBones_) {
        // Yellow sphere at hand bone world position
        se::DebugRenderer::Get().DrawSphere(handWorldPos, 0.05f, glm::vec3(1.0f, 1.0f, 0.0f));
        // Green sphere at rifle target position
        se::DebugRenderer::Get().DrawSphere(rifleWorldPos, 0.04f, glm::vec3(0.0f, 1.0f, 0.0f));
    }
    
    // Apply left hand IK if in aim mode
    if (currentMode_ == LocomotionMode::Aiming) {
        ApplyLeftHandIK();
    }
}

void AdvancedCharacterAnimator::ApplyLeftHandIK() {
    if (!animatorComp_ || !animatorComp_->animator || !modelData_) {
        return;
    }
    
    // For now, the rifle hold animations already have proper hand positions
    // A full IK system would:
    // 1. Calculate foregrip world position from rifle transform + rifleLeftHandTarget_
    // 2. Run two-bone IK solver for left arm (shoulder -> elbow -> hand)
    // 3. Apply resulting bone rotations to finalPose_
    
    // This is a placeholder - the aim animations (MM_Rifle_*) already include
    // proper hand positions for rifle holding, so IK may not be strictly necessary
    // If the hands don't align properly, this function can be extended with a
    // proper two-bone IK solver.
    
    // Debug: Draw foregrip target position
    if (debugDrawBones_ && rifleEntity_.IsValid()) {
        auto& rifleTransform = rifleEntity_.GetComponent<se::TransformComponent>();
        glm::mat4 rifleWorld = rifleTransform.WorldMatrix;
        
        // Calculate foregrip world position
        glm::vec3 foregrip = glm::vec3(rifleWorld * glm::vec4(rifleLeftHandTarget_, 1.0f));
        
        // Draw a sphere at the foregrip target
        se::DebugRenderer::Get().DrawSphere(foregrip, 0.03f, glm::vec3(0.0f, 0.5f, 1.0f));
    }
}

} // namespace AnimationTest

