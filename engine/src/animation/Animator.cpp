#include "engine/animation/Animator.h"

#include <gtc/matrix_transform.hpp>
#include <gtx/quaternion.hpp>
#include <gtx/matrix_decompose.hpp>

#include "engine/Log.h"

namespace se {

Animator::Animator(const SkinnedModelData* modelData) : modelData_(modelData) {
    if (modelData_) {
        size_t boneCount = modelData_->Bones.size();
        finalBoneMatrices_.resize(boneCount, glm::mat4(1.0f));
        localTransforms_.resize(boneCount, glm::mat4(1.0f));
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
        return;
    }
    
    currentClip_ = clip;
    looping_ = loop;
    currentTime_ = 0.0f;
    playing_ = true;
    paused_ = false;
    
    // Cancel any active blend
    isBlending_ = false;
    blendFromClip_ = nullptr;
    blendWeight_ = 1.0f;
}

void Animator::Crossfade(std::shared_ptr<AnimationClip> clip, float duration, bool loop) {
    if (!clip) {
        return;
    }
    
    // If same clip, ignore
    if (currentClip_ == clip) {
        return;
    }
    
    // If not currently playing, just play immediately
    if (!playing_ || !currentClip_) {
        Play(clip, loop);
        return;
    }
    
    // Store current animation as blend source
    blendFromClip_ = currentClip_;
    blendFromTime_ = currentTime_;
    
    // Start new animation
    currentClip_ = clip;
    currentTime_ = 0.0f;
    looping_ = loop;
    
    // Setup blend
    blendDuration_ = duration > 0.0f ? duration : 0.01f;
    blendWeight_ = 0.0f;
    isBlending_ = true;
}

void Animator::Stop() {
    playing_ = false;
    paused_ = false;
    currentTime_ = 0.0f;
    isBlending_ = false;
    blendFromClip_ = nullptr;
    
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
    
    if (isBlending_) {
        CalculateBoneTransformsBlended();
    } else {
        CalculateBoneTransforms();
    }
}

void Animator::Update(float deltaTime) {
    if (!playing_ || paused_ || !currentClip_ || !modelData_) return;
    
    float ticksPerSecond = currentClip_->GetTicksPerSecond();
    float duration = currentClip_->GetDuration();
    
    // Advance current animation time
    currentTime_ += deltaTime * ticksPerSecond * speed_;
    
    if (currentTime_ >= duration) {
        if (looping_) {
            currentTime_ = fmod(currentTime_, duration);
        } else {
            currentTime_ = duration;
            playing_ = false;
        }
    }
    
    // Update blend state
    if (isBlending_) {
        blendWeight_ += deltaTime / blendDuration_;
        
        if (blendWeight_ >= 1.0f) {
            blendWeight_ = 1.0f;
            isBlending_ = false;
            blendFromClip_ = nullptr;
        }
        
        // Also advance blend source animation
        if (blendFromClip_) {
            float blendTicksPerSecond = blendFromClip_->GetTicksPerSecond();
            float blendDuration = blendFromClip_->GetDuration();
            blendFromTime_ += deltaTime * blendTicksPerSecond * speed_;
            if (blendFromTime_ >= blendDuration) {
                blendFromTime_ = fmod(blendFromTime_, blendDuration);
            }
        }
        
        CalculateBoneTransformsBlended();
    } else {
        CalculateBoneTransforms();
    }
}

void Animator::CalculateBoneTransforms() {
    if (!modelData_ || !currentClip_) return;
    
    for (size_t i = 0; i < modelData_->Bones.size(); ++i) {
        if (modelData_->Bones[i].ParentIndex < 0) {
            ProcessBoneHierarchy(static_cast<int>(i), glm::mat4(1.0f));
        }
    }
}

void Animator::CalculateBoneTransformsBlended() {
    if (!modelData_ || !currentClip_) return;
    
    for (size_t i = 0; i < modelData_->Bones.size(); ++i) {
        if (modelData_->Bones[i].ParentIndex < 0) {
            ProcessBoneHierarchyBlended(static_cast<int>(i), glm::mat4(1.0f));
        }
    }
}

void Animator::ProcessBoneHierarchy(int boneIndex, const glm::mat4& parentTransform) {
    if (boneIndex < 0 || boneIndex >= static_cast<int>(modelData_->Bones.size())) return;
    
    const BoneInfo& bone = modelData_->Bones[boneIndex];
    glm::mat4 localTransform = GetBoneLocalTransform(currentClip_.get(), bone.Name, currentTime_);
    
    glm::mat4 globalTransform = parentTransform * localTransform;
    
    finalBoneMatrices_[boneIndex] = modelData_->GlobalInverseTransform * globalTransform * bone.OffsetMatrix;
    localTransforms_[boneIndex] = localTransform;
    
    for (size_t i = 0; i < modelData_->Bones.size(); ++i) {
        if (modelData_->Bones[i].ParentIndex == boneIndex) {
            ProcessBoneHierarchy(static_cast<int>(i), globalTransform);
        }
    }
}

void Animator::ProcessBoneHierarchyBlended(int boneIndex, const glm::mat4& parentTransform) {
    if (boneIndex < 0 || boneIndex >= static_cast<int>(modelData_->Bones.size())) return;
    
    const BoneInfo& bone = modelData_->Bones[boneIndex];
    
    // Get transforms from both animations
    glm::mat4 fromTransform = blendFromClip_ 
        ? GetBoneLocalTransform(blendFromClip_.get(), bone.Name, blendFromTime_)
        : GetBoneLocalTransform(currentClip_.get(), bone.Name, currentTime_);
    
    glm::mat4 toTransform = GetBoneLocalTransform(currentClip_.get(), bone.Name, currentTime_);
    
    // Decompose matrices for proper interpolation
    glm::vec3 fromPos, toPos, fromScale, toScale;
    glm::quat fromRot, toRot;
    glm::vec3 skew;
    glm::vec4 perspective;
    
    glm::decompose(fromTransform, fromScale, fromRot, fromPos, skew, perspective);
    glm::decompose(toTransform, toScale, toRot, toPos, skew, perspective);
    
    // Lerp/slerp between transforms
    glm::vec3 blendedPos = glm::mix(fromPos, toPos, blendWeight_);
    glm::quat blendedRot = glm::slerp(fromRot, toRot, blendWeight_);
    glm::vec3 blendedScale = glm::mix(fromScale, toScale, blendWeight_);
    
    // Reconstruct blended local transform
    glm::mat4 localTransform = glm::translate(glm::mat4(1.0f), blendedPos)
                             * glm::toMat4(blendedRot)
                             * glm::scale(glm::mat4(1.0f), blendedScale);
    
    glm::mat4 globalTransform = parentTransform * localTransform;
    
    finalBoneMatrices_[boneIndex] = modelData_->GlobalInverseTransform * globalTransform * bone.OffsetMatrix;
    localTransforms_[boneIndex] = localTransform;
    
    for (size_t i = 0; i < modelData_->Bones.size(); ++i) {
        if (modelData_->Bones[i].ParentIndex == boneIndex) {
            ProcessBoneHierarchyBlended(static_cast<int>(i), globalTransform);
        }
    }
}

glm::mat4 Animator::GetBoneLocalTransform(const AnimationClip* clip, const std::string& boneName, float time) const {
    if (!clip) {
        int boneIndex = modelData_->GetBoneIndex(boneName);
        if (boneIndex >= 0) {
            return modelData_->Bones[boneIndex].LocalTransform;
        }
        return glm::mat4(1.0f);
    }
    
    const AnimationChannel* channel = clip->FindChannel(boneName);
    
    if (!channel) {
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

glm::mat4 Animator::GetBoneWorldMatrix(int boneIndex) const {
    if (!modelData_ || boneIndex < 0 || boneIndex >= static_cast<int>(finalBoneMatrices_.size())) {
        return glm::mat4(1.0f);
    }
    
    // finalBoneMatrices_ contains: GlobalInverseTransform * GlobalTransform * OffsetMatrix
    // We need to undo the GlobalInverseTransform and OffsetMatrix to get world transform
    // WorldTransform = inverse(GlobalInverseTransform) * finalBoneMatrix * inverse(OffsetMatrix)
    const BoneInfo& bone = modelData_->Bones[boneIndex];
    glm::mat4 globalTransform = glm::inverse(modelData_->GlobalInverseTransform);
    glm::mat4 boneWorld = globalTransform * finalBoneMatrices_[boneIndex] * glm::inverse(bone.OffsetMatrix);
    
    return boneWorld;
}

glm::mat4 Animator::GetBoneWorldMatrix(const std::string& boneName) const {
    if (!modelData_) return glm::mat4(1.0f);
    
    int boneIndex = modelData_->GetBoneIndex(boneName);
    return GetBoneWorldMatrix(boneIndex);
}

}  // namespace se
