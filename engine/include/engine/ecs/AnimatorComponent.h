#pragma once

#include <memory>
#include <string>

#include "engine/animation/Animator.h"
#include "engine/animation/AnimationClip.h"
#include "engine/animation/AnimatorController.h"
#include "engine/resources/ModelData.h"

namespace se {

// Component for entities with skeletal animation.
// Supports simple clip playback or full state machine via AnimatorController.
// AnimationSystem auto-ticks this component every frame.
struct AnimatorComponent {
    std::unique_ptr<Animator> animator;
    std::shared_ptr<AnimationClip> currentClip;
    std::shared_ptr<SkinnedModelData> modelData;
    std::shared_ptr<AnimatorController> controller;
    
    bool autoPlay = true;
    bool playing = false;
    float speed = 1.0f;
    bool loop = true;
    
    // State machine state
    std::string currentStateName;
    
    AnimatorComponent() = default;
    AnimatorComponent(const AnimatorComponent&) = delete;
    AnimatorComponent& operator=(const AnimatorComponent&) = delete;
    AnimatorComponent(AnimatorComponent&&) = default;
    AnimatorComponent& operator=(AnimatorComponent&&) = default;
    
    // Initialize with model skeleton data
    void Init(std::shared_ptr<SkinnedModelData> data) {
        modelData = data;
        if (modelData) {
            animator = std::make_unique<Animator>(modelData.get());
        }
    }
    
    // Set an AnimatorController for state machine animation
    void SetController(std::shared_ptr<AnimatorController> ctrl) {
        controller = ctrl;
        if (controller && animator) {
            currentStateName = controller->GetDefaultStateName();
            auto* state = controller->GetState(currentStateName);
            if (state && state->Clip) {
                Play(state->Clip, state->Loop);
                speed = state->Speed;
            }
        }
    }
    
    // Simple clip playback (works without controller)
    void Play(std::shared_ptr<AnimationClip> clip, bool loopAnim = true) {
        currentClip = clip;
        loop = loopAnim;
        if (animator && clip) {
            animator->Play(clip, loopAnim);
            playing = true;
        }
    }
    
    // High-level: Load and play animation from file path
    bool PlayClip(const std::string& path, bool loopAnim = true);
    
    // Crossfade to a new clip with smooth transition
    void CrossfadeTo(std::shared_ptr<AnimationClip> clip, float duration = 0.25f, bool loopAnim = true) {
        if (animator && clip) {
            animator->Crossfade(clip, duration, loopAnim);
            currentClip = clip;
            loop = loopAnim;
            playing = true;
        }
    }
    
    // Load and crossfade to animation from path
    bool CrossfadeToClip(const std::string& path, float duration = 0.25f, bool loopAnim = true);
    
    void Stop() {
        if (animator) {
            animator->Stop();
            playing = false;
        }
    }
    
    void Pause() {
        if (animator) {
            animator->Pause();
        }
    }
    
    void Resume() {
        if (animator) {
            animator->Resume();
        }
    }
    
    // Called by AnimationSystem (not by user code)
    void Update(float deltaTime) {
        if (!animator || !playing) return;
        
        animator->SetSpeed(speed);
        animator->Update(deltaTime);
        
        // Check state machine transitions
        if (controller && !currentStateName.empty()) {
            float normalizedTime = GetNormalizedTime();
            auto* transition = controller->FindTransition(currentStateName, normalizedTime);
            if (transition) {
                TransitionTo(transition->ToState, transition->TransitionDuration);
            }
        }
    }
    
    // State machine parameter control
    void SetBool(const std::string& name, bool value) {
        if (controller) controller->SetBool(name, value);
    }
    
    void SetFloat(const std::string& name, float value) {
        if (controller) controller->SetFloat(name, value);
    }
    
    void SetInt(const std::string& name, int value) {
        if (controller) controller->SetInt(name, value);
    }
    
    void SetTrigger(const std::string& name) {
        if (controller) controller->SetTrigger(name);
    }
    
    // Force transition to a specific state (with crossfade)
    void TransitionTo(const std::string& stateName, float transitionDuration = 0.25f) {
        if (!controller || !animator) return;
        
        auto* state = controller->GetState(stateName);
        if (state && state->Clip) {
            currentStateName = stateName;
            speed = state->Speed;
            CrossfadeTo(state->Clip, transitionDuration, state->Loop);
        }
    }
    
    const std::vector<glm::mat4>& GetBoneMatrices() const {
        static std::vector<glm::mat4> empty;
        return animator ? animator->GetBoneMatrices() : empty;
    }
    
    bool HasBones() const {
        return modelData && modelData->HasSkeleton();
    }
    
    float GetNormalizedTime() const {
        if (!animator || !currentClip) return 0.0f;
        float duration = currentClip->GetDurationInSeconds();
        if (duration <= 0.0f) return 0.0f;
        return animator->GetCurrentTime() / duration;
    }
    
    const std::string& GetCurrentStateName() const {
        return currentStateName;
    }
};

}  // namespace se
