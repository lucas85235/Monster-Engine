#include "engine/animation/Animator.h"

#include <gtc/matrix_transform.hpp>
#include <gtx/quaternion.hpp>

#include "engine/Log.h"

namespace se {

Animator::Animator(const SkinnedModelData* modelData) : modelData_(modelData) {
    if (modelData_) {
        size_t boneCount = modelData_->Bones.size();
        finalBoneMatrices_.resize(boneCount, glm::mat4(1.0f));
        localTransforms_.resize(boneCount, glm::mat4(1.0f));
        SE_LOG_INFO("Animator: Initialized with {} bones", boneCount);
    }
}

void Animator::SetModelData(const SkinnedModelData* modelData) {
    modelData_ = modelData;
    if (modelData_) {
        size_t boneCount = modelData_->Bones.size();
        finalBoneMatrices_.resize(boneCount, glm::mat4(1.0f));
        localTransforms_.resize(boneCount, glm::mat4(1.0f));
    }
}

void Animator::Play(std::shared_ptr<AnimationClip> clip, bool loop) {
    if (!clip) {
        SE_LOG_WARN("Animator::Play: null clip");
        return;
    }
    
    currentClip_ = clip;
    looping_ = loop;
    currentTime_ = 0.0f;
    playing_ = true;
    paused_ = false;
    
    SE_LOG_INFO("Animator: Playing '{}' (duration: {:.2f}s, loop: {})", 
                clip->GetName(), clip->GetDurationInSeconds(), loop);
}

void Animator::Stop() {
    playing_ = false;
    paused_ = false;
    currentTime_ = 0.0f;
    
    for (auto& mat : finalBoneMatrices_) {
        mat = glm::mat4(1.0f);
    }
}

void Animator::Pause() {
    if (playing_) paused_ = true;
}

void Animator::Resume() {
    paused_ = false;
}

void Animator::SetTime(float time) {
    if (!currentClip_) return;
    
    float duration = currentClip_->GetDuration();
    currentTime_ = fmod(time * currentClip_->GetTicksPerSecond(), duration);
    if (currentTime_ < 0.0f) currentTime_ += duration;
    
    CalculateBoneTransforms();
}

void Animator::Update(float deltaTime) {
    if (!playing_ || paused_ || !currentClip_ || !modelData_) return;
    
    float ticksPerSecond = currentClip_->GetTicksPerSecond();
    float duration = currentClip_->GetDuration();
    
    currentTime_ += deltaTime * ticksPerSecond * speed_;
    
    if (currentTime_ >= duration) {
        if (looping_) {
            currentTime_ = fmod(currentTime_, duration);
        } else {
            currentTime_ = duration;
            playing_ = false;
        }
    }
    
    CalculateBoneTransforms();
}

void Animator::CalculateBoneTransforms() {
    if (!modelData_ || !currentClip_) return;
    
    // Find root bones (bones with no parent)
    for (size_t i = 0; i < modelData_->Bones.size(); ++i) {
        if (modelData_->Bones[i].ParentIndex < 0) {
            // Start with identity, GlobalInverseTransform is applied at the end
            ProcessBoneHierarchy(static_cast<int>(i), glm::mat4(1.0f));
        }
    }
}

void Animator::ProcessBoneHierarchy(int boneIndex, const glm::mat4& parentTransform) {
    if (boneIndex < 0 || boneIndex >= static_cast<int>(modelData_->Bones.size())) return;
    
    const BoneInfo& bone = modelData_->Bones[boneIndex];
    glm::mat4 localTransform = GetBoneLocalTransform(bone.Name, currentTime_);
    
    glm::mat4 globalTransform = parentTransform * localTransform;
    
    // Final transform = GlobalInverseTransform * GlobalTransform * OffsetMatrix
    finalBoneMatrices_[boneIndex] = modelData_->GlobalInverseTransform * globalTransform * bone.OffsetMatrix;
    localTransforms_[boneIndex] = localTransform;
    
    // Process children
    for (size_t i = 0; i < modelData_->Bones.size(); ++i) {
        if (modelData_->Bones[i].ParentIndex == boneIndex) {
            ProcessBoneHierarchy(static_cast<int>(i), globalTransform);
        }
    }
}

glm::mat4 Animator::GetBoneLocalTransform(const std::string& boneName, float time) const {
    const AnimationChannel* channel = currentClip_->FindChannel(boneName);
    
    if (!channel) {
        // Use the bone's stored local transform from the hierarchy
        int boneIndex = modelData_->GetBoneIndex(boneName);
        if (boneIndex >= 0) {
            return modelData_->Bones[boneIndex].LocalTransform;
        }
        return glm::mat4(1.0f);
    }
    
    glm::vec3 position = channel->GetPosition(time);
    glm::quat rotation = channel->GetRotation(time);
    glm::vec3 scale = channel->GetScale(time);
    
    glm::mat4 translationMatrix = glm::translate(glm::mat4(1.0f), position);
    glm::mat4 rotationMatrix = glm::toMat4(rotation);
    glm::mat4 scaleMatrix = glm::scale(glm::mat4(1.0f), scale);
    
    return translationMatrix * rotationMatrix * scaleMatrix;
}

}  // namespace se
