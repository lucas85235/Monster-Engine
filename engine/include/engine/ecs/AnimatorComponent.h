#pragma once

#include <memory>

#include "engine/animation/Animator.h"
#include "engine/animation/AnimationClip.h"
#include "engine/resources/ModelData.h"

namespace se {

// Component for entities with skeletal animation
struct AnimatorComponent {
    std::unique_ptr<Animator> animator;
    std::shared_ptr<AnimationClip> currentClip;
    std::shared_ptr<SkinnedModelData> modelData;
    
    bool autoPlay = true;
    bool playing = false;
    float speed = 1.0f;
    bool loop = true;
    
    AnimatorComponent() = default;
    AnimatorComponent(const AnimatorComponent&) = delete;
    AnimatorComponent& operator=(const AnimatorComponent&) = delete;
    AnimatorComponent(AnimatorComponent&&) = default;
    AnimatorComponent& operator=(AnimatorComponent&&) = default;
    
    void Init(std::shared_ptr<SkinnedModelData> data) {
        modelData = data;
        if (modelData) {
            animator = std::make_unique<Animator>(modelData.get());
        }
    }
    
    void Play(std::shared_ptr<AnimationClip> clip, bool loopAnim = true) {
        currentClip = clip;
        loop = loopAnim;
        if (animator && clip) {
            animator->Play(clip, loopAnim);
            playing = true;
        }
    }
    
    void Stop() {
        if (animator) {
            animator->Stop();
            playing = false;
        }
    }
    
    void Update(float deltaTime) {
        if (animator && playing) {
            animator->SetSpeed(speed);
            animator->Update(deltaTime);
        }
    }
    
    const std::vector<glm::mat4>& GetBoneMatrices() const {
        static std::vector<glm::mat4> empty;
        return animator ? animator->GetBoneMatrices() : empty;
    }
    
    bool HasBones() const {
        return modelData && modelData->HasSkeleton();
    }
};

}  // namespace se
