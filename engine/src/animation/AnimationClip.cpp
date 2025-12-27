#include "engine/animation/AnimationClip.h"

#include <algorithm>

namespace se {

namespace {
template <typename T>
size_t FindKeyframeIndex(const std::vector<T>& keys, float time) {
    if (keys.empty()) return 0;
    for (size_t i = 0; i < keys.size() - 1; ++i) {
        if (time < keys[i + 1].Time) return i;
    }
    return keys.size() - 1;
}

float GetInterpolationFactor(float t1, float t2, float time) {
    float delta = t2 - t1;
    if (delta < 0.0001f) return 0.0f;
    return (time - t1) / delta;
}
}  // namespace

glm::vec3 AnimationChannel::GetPosition(float time) const {
    if (PositionKeys.empty()) return glm::vec3(0.0f);
    if (PositionKeys.size() == 1) return PositionKeys[0].Value;
    
    size_t index = FindKeyframeIndex(PositionKeys, time);
    size_t nextIndex = std::min(index + 1, PositionKeys.size() - 1);
    
    float factor = GetInterpolationFactor(
        PositionKeys[index].Time, 
        PositionKeys[nextIndex].Time, 
        time
    );
    
    return glm::mix(PositionKeys[index].Value, PositionKeys[nextIndex].Value, factor);
}

glm::quat AnimationChannel::GetRotation(float time) const {
    if (RotationKeys.empty()) return glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    if (RotationKeys.size() == 1) return RotationKeys[0].Value;
    
    size_t index = FindKeyframeIndex(RotationKeys, time);
    size_t nextIndex = std::min(index + 1, RotationKeys.size() - 1);
    
    float factor = GetInterpolationFactor(
        RotationKeys[index].Time, 
        RotationKeys[nextIndex].Time, 
        time
    );
    
    return glm::slerp(RotationKeys[index].Value, RotationKeys[nextIndex].Value, factor);
}

glm::vec3 AnimationChannel::GetScale(float time) const {
    if (ScaleKeys.empty()) return glm::vec3(1.0f);
    if (ScaleKeys.size() == 1) return ScaleKeys[0].Value;
    
    size_t index = FindKeyframeIndex(ScaleKeys, time);
    size_t nextIndex = std::min(index + 1, ScaleKeys.size() - 1);
    
    float factor = GetInterpolationFactor(
        ScaleKeys[index].Time, 
        ScaleKeys[nextIndex].Time, 
        time
    );
    
    return glm::mix(ScaleKeys[index].Value, ScaleKeys[nextIndex].Value, factor);
}

AnimationClip::AnimationClip(const std::string& name, float duration, float ticksPerSecond)
    : name_(name), duration_(duration), ticksPerSecond_(ticksPerSecond) {
    if (ticksPerSecond_ < 1.0f) ticksPerSecond_ = 24.0f;
}

void AnimationClip::AddChannel(AnimationChannel channel) {
    auto it = channelMap_.find(channel.BoneName);
    if (it != channelMap_.end()) {
        // Merge keyframes with existing channel - use the one with more keyframes
        auto& existing = channels_[it->second];
        if (channel.PositionKeys.size() > existing.PositionKeys.size()) {
            existing.PositionKeys = std::move(channel.PositionKeys);
        }
        if (channel.RotationKeys.size() > existing.RotationKeys.size()) {
            existing.RotationKeys = std::move(channel.RotationKeys);
        }
        if (channel.ScaleKeys.size() > existing.ScaleKeys.size()) {
            existing.ScaleKeys = std::move(channel.ScaleKeys);
        }
    } else {
        channelMap_[channel.BoneName] = channels_.size();
        channels_.push_back(std::move(channel));
    }
}

const AnimationChannel* AnimationClip::FindChannel(const std::string& boneName) const {
    auto it = channelMap_.find(boneName);
    if (it != channelMap_.end()) {
        return &channels_[it->second];
    }
    return nullptr;
}

}  // namespace se
