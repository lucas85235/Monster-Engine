#include "engine/animation/locomotion/LocomotionController.h"
#include "engine/animation/AnimationManager.h"
#include "engine/Log.h"

#include <cmath>

namespace se {
namespace anim {

void LocomotionController::Initialize(const LocomotionConfig& config, const SkinnedModelData* skeleton) {
    config_ = config;
    skeleton_ = skeleton;
    
    if (!skeleton_) {
        SE_LOG_ERROR("[LocomotionController] Cannot initialize without skeleton");
        return;
    }
    
    SetupBlendSpaces();
    
    initialized_ = true;
    SE_LOG_INFO("[LocomotionController] Initialized successfully");
}

void LocomotionController::SetupBlendSpaces() {
    // Create 1D locomotion blend space (idle -> walk -> run)
    locomotionBlendSpace_ = std::make_unique<BlendSpace1D>("Locomotion");
    locomotionBlendSpace_->SetBounds(0.0f, config_.maxSpeed);
    
    // Load and add animations
    if (!config_.animations.idle.empty()) {
        auto clip = AnimationManager::Load(config_.animations.idle);
        if (clip) {
            locomotionBlendSpace_->AddSample(clip, 0.0f);
            SE_LOG_INFO("[LocomotionController] Added idle animation");
        }
    }
    
    if (!config_.animations.walk.empty()) {
        auto clip = AnimationManager::Load(config_.animations.walk);
        if (clip) {
            locomotionBlendSpace_->AddSample(clip, config_.walkThreshold);
            SE_LOG_INFO("[LocomotionController] Added walk animation at {}", config_.walkThreshold);
        }
    }
    
    if (!config_.animations.run.empty()) {
        auto clip = AnimationManager::Load(config_.animations.run);
        if (clip) {
            locomotionBlendSpace_->AddSample(clip, config_.runThreshold);
            SE_LOG_INFO("[LocomotionController] Added run animation at {}", config_.runThreshold);
        }
    }
    
    // Create 2D strafe blend space (for aiming mode)
    strafeBlendSpace_ = std::make_unique<BlendSpace2D>("Strafe");
    strafeBlendSpace_->SetBounds(glm::vec2(-1.0f), glm::vec2(1.0f));
    
    // Center (idle while aiming)
    if (!config_.animations.strafeIdle.empty()) {
        auto clip = AnimationManager::Load(config_.animations.strafeIdle);
        if (clip) {
            strafeBlendSpace_->AddSample(clip, glm::vec2(0.0f, 0.0f));
        }
    }
    
    // Cardinal directions
    if (!config_.animations.strafeForward.empty()) {
        auto clip = AnimationManager::Load(config_.animations.strafeForward);
        if (clip) {
            strafeBlendSpace_->AddSample(clip, glm::vec2(0.0f, 1.0f));
        }
    }
    
    if (!config_.animations.strafeBack.empty()) {
        auto clip = AnimationManager::Load(config_.animations.strafeBack);
        if (clip) {
            strafeBlendSpace_->AddSample(clip, glm::vec2(0.0f, -1.0f));
        }
    }
    
    if (!config_.animations.strafeLeft.empty()) {
        auto clip = AnimationManager::Load(config_.animations.strafeLeft);
        if (clip) {
            strafeBlendSpace_->AddSample(clip, glm::vec2(-1.0f, 0.0f));
        }
    }
    
    if (!config_.animations.strafeRight.empty()) {
        auto clip = AnimationManager::Load(config_.animations.strafeRight);
        if (clip) {
            strafeBlendSpace_->AddSample(clip, glm::vec2(1.0f, 0.0f));
        }
    }
    
    SE_LOG_INFO("[LocomotionController] Blend spaces configured: locomotion={} samples, strafe={} samples",
                locomotionBlendSpace_->GetSampleCount(), strafeBlendSpace_->GetSampleCount());
}

void LocomotionController::SetMode(LocomotionMode mode) {
    if (mode == currentMode_) return;
    
    LocomotionMode oldMode = currentMode_;
    targetMode_ = mode;
    modeTransitionWeight_ = 0.0f;
    
    SE_LOG_INFO("[LocomotionController] Mode transition: {} -> {}",
                static_cast<int>(currentMode_), static_cast<int>(mode));
    
    if (modeChangedCallback_) {
        modeChangedCallback_(oldMode, mode);
    }
}

void LocomotionController::SetVelocity(float speed) {
    targetVelocity_ = speed;
}

void LocomotionController::SetStrafeInput(const glm::vec2& input) {
    strafeInput_ = input;
}

void LocomotionController::Update(float dt, Pose& outPose) {
    if (!initialized_) return;
    
    UpdateVelocity(dt);
    UpdateModeTransition(dt);
    
    animationTime_ += dt;
    
    // Evaluate based on mode
    bool needsBlend = (modeTransitionWeight_ < 0.99f && modeTransitionWeight_ > 0.01f);
    
    if (currentMode_ == LocomotionMode::Standing || 
        (needsBlend && targetMode_ == LocomotionMode::Standing)) {
        // Sample locomotion blend space
        if (locomotionBlendSpace_ && skeleton_) {
            standingPose_.Resize(outPose.GetBoneCount());
            locomotionBlendSpace_->Evaluate(currentVelocity_, standingPose_, animationTime_, skeleton_);
        }
    }
    
    if (currentMode_ == LocomotionMode::Aiming || 
        (needsBlend && targetMode_ == LocomotionMode::Aiming)) {
        // Sample strafe blend space
        if (strafeBlendSpace_ && skeleton_) {
            strafePose_.Resize(outPose.GetBoneCount());
            strafeBlendSpace_->Evaluate(currentStrafeInput_, strafePose_, animationTime_, skeleton_);
        }
    }
    
    // Output final pose based on mode
    if (needsBlend) {
        // Blend between modes during transition
        if (targetMode_ == LocomotionMode::Aiming) {
            outPose = standingPose_;
            outPose.BlendWith(strafePose_, modeTransitionWeight_);
        } else {
            outPose = strafePose_;
            outPose.BlendWith(standingPose_, modeTransitionWeight_);
        }
    } else {
        // Use current mode's pose directly
        switch (currentMode_) {
            case LocomotionMode::Standing:
                outPose = standingPose_;
                break;
            case LocomotionMode::Aiming:
                outPose = strafePose_;
                break;
            default:
                outPose = standingPose_;
                break;
        }
    }
}

void LocomotionController::UpdateVelocity(float dt) {
    // Smooth velocity
    float t = 1.0f - std::exp(-config_.velocityLerpSpeed * dt);
    currentVelocity_ = currentVelocity_ + (targetVelocity_ - currentVelocity_) * t;
    
    // Snap to zero if very close
    if (currentVelocity_ < config_.idleThreshold) {
        currentVelocity_ = 0.0f;
    }
    
    // Smooth strafe input
    t = 1.0f - std::exp(-config_.strafeLerpSpeed * dt);
    currentStrafeInput_.x = currentStrafeInput_.x + (strafeInput_.x - currentStrafeInput_.x) * t;
    currentStrafeInput_.y = currentStrafeInput_.y + (strafeInput_.y - currentStrafeInput_.y) * t;
    
    // Snap to zero
    if (glm::length(currentStrafeInput_) < 0.05f) {
        currentStrafeInput_ = glm::vec2(0.0f);
    }
}

void LocomotionController::UpdateModeTransition(float dt) {
    if (targetMode_ != currentMode_) {
        float transitionSpeed = 1.0f / config_.modeTransitionDuration;
        modeTransitionWeight_ += transitionSpeed * dt;
        
        if (modeTransitionWeight_ >= 1.0f) {
            modeTransitionWeight_ = 1.0f;
            currentMode_ = targetMode_;
        }
    }
}

}  // namespace anim
}  // namespace se
