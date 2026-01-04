#include "engine/animation/advanced/Pose.h"

#include "engine/animation/AnimationClip.h"
#include "engine/resources/ModelData.h"
#include "engine/Log.h"

namespace se::anim {

BoneTransform BoneTransform::Blend(const BoneTransform& a, const BoneTransform& b, float t) {
    BoneTransform result;
    result.position = glm::mix(a.position, b.position, t);
    result.scale = glm::mix(a.scale, b.scale, t);
    
    // Use slerp for rotation, ensure shortest path
    float dot = glm::dot(a.rotation, b.rotation);
    glm::quat bRot = b.rotation;
    if (dot < 0.0f) {
        bRot = -bRot;
        dot = -dot;
    }
    
    // For very close quaternions, use nlerp (faster)
    if (dot > 0.9995f) {
        result.rotation = glm::normalize(glm::mix(a.rotation, bRot, t));
    } else {
        result.rotation = glm::slerp(a.rotation, bRot, t);
    }
    
    return result;
}

BoneTransform BoneTransform::BlendAdditive(const BoneTransform& base, const BoneTransform& additive, float weight) {
    BoneTransform result;
    
    // Additive position/scale: base + (additive - identity) * weight
    result.position = base.position + additive.position * weight;
    result.scale = base.scale + (additive.scale - glm::vec3(1.0f)) * weight;
    
    // Additive rotation: base * (additive ^ weight)
    // For small weights, slerp from identity to additive, then multiply
    glm::quat additiveRotation = glm::slerp(glm::quat(1.0f, 0.0f, 0.0f, 0.0f), additive.rotation, weight);
    result.rotation = glm::normalize(base.rotation * additiveRotation);
    
    return result;
}

Pose::Pose(size_t boneCount) {
    Resize(boneCount);
}

Pose::Pose(const SkinnedModelData* skeleton) {
    if (skeleton) {
        Resize(skeleton->Bones.size());
    }
}

void Pose::Resize(size_t boneCount) {
    transforms_.resize(boneCount);
    SetIdentity();
}

void Pose::SetIdentity() {
    for (auto& transform : transforms_) {
        transform = BoneTransform::Identity();
    }
}

void Pose::SetFromClip(const AnimationClip* clip, float time, const SkinnedModelData* skeleton) {
    if (!clip || !skeleton || transforms_.empty()) {
        return;
    }
    
    // Convert time to ticks
    float ticksPerSecond = clip->GetTicksPerSecond();
    if (ticksPerSecond <= 0.0f) {
        ticksPerSecond = 24.0f;
    }
    float timeInTicks = time * ticksPerSecond;
    
    // Loop if needed
    float duration = clip->GetDuration();
    if (duration > 0.0f) {
        timeInTicks = fmod(timeInTicks, duration);
        if (timeInTicks < 0.0f) {
            timeInTicks += duration;
        }
    }
    
    // Sample each bone
    for (size_t i = 0; i < skeleton->Bones.size(); ++i) {
        const auto& boneInfo = skeleton->Bones[i];
        const AnimationChannel* channel = clip->FindChannel(boneInfo.Name);
        
        if (channel) {
            transforms_[i].position = channel->GetPosition(timeInTicks);
            transforms_[i].rotation = channel->GetRotation(timeInTicks);
            transforms_[i].scale = channel->GetScale(timeInTicks);
        } else {
            transforms_[i] = BoneTransform::Identity();
        }
    }
}

void Pose::SetFromClipNormalized(const AnimationClip* clip, float normalizedTime, const SkinnedModelData* skeleton) {
    if (!clip || !skeleton || transforms_.empty()) {
        return;
    }
    
    // Ensure normalized time is in [0, 1)
    normalizedTime = fmod(normalizedTime, 1.0f);
    if (normalizedTime < 0.0f) {
        normalizedTime += 1.0f;
    }
    
    // Convert normalized time to ticks using clip's duration
    float duration = clip->GetDuration();
    if (duration <= 0.0f) {
        duration = 24.0f;  // Fallback
    }
    float timeInTicks = normalizedTime * duration;
    
    // Sample each bone
    for (size_t i = 0; i < skeleton->Bones.size(); ++i) {
        const auto& boneInfo = skeleton->Bones[i];
        const AnimationChannel* channel = clip->FindChannel(boneInfo.Name);
        
        if (channel) {
            transforms_[i].position = channel->GetPosition(timeInTicks);
            transforms_[i].rotation = channel->GetRotation(timeInTicks);
            transforms_[i].scale = channel->GetScale(timeInTicks);
        } else {
            transforms_[i] = BoneTransform::Identity();
        }
    }
}

void Pose::BlendWith(const Pose& other, float weight) {
    if (other.transforms_.size() != transforms_.size()) {
        SE_LOG_WARN("[Pose] BlendWith: Pose size mismatch ({} vs {})", transforms_.size(), other.transforms_.size());
        return;
    }
    
    weight = glm::clamp(weight, 0.0f, 1.0f);
    
    for (size_t i = 0; i < transforms_.size(); ++i) {
        transforms_[i] = BoneTransform::Blend(transforms_[i], other.transforms_[i], weight);
    }
}

void Pose::ApplyAdditive(const Pose& additive, float weight) {
    if (additive.transforms_.size() != transforms_.size()) {
        SE_LOG_WARN("[Pose] ApplyAdditive: Pose size mismatch ({} vs {})", transforms_.size(), additive.transforms_.size());
        return;
    }
    
    for (size_t i = 0; i < transforms_.size(); ++i) {
        transforms_[i] = BoneTransform::BlendAdditive(transforms_[i], additive.transforms_[i], weight);
    }
}

}  // namespace se::anim
