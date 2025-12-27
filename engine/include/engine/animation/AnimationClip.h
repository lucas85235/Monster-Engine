#pragma once

#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace se {

// Position/scale keyframe
struct VectorKeyframe {
    float Time = 0.0f;
    glm::vec3 Value{0.0f};
};

// Rotation keyframe
struct QuatKeyframe {
    float Time = 0.0f;
    glm::quat Value{1.0f, 0.0f, 0.0f, 0.0f};
};

// Animation channel for a single bone
struct AnimationChannel {
    std::string BoneName;
    std::vector<VectorKeyframe> PositionKeys;
    std::vector<QuatKeyframe> RotationKeys;
    std::vector<VectorKeyframe> ScaleKeys;
    
    glm::vec3 GetPosition(float time) const;
    glm::quat GetRotation(float time) const;
    glm::vec3 GetScale(float time) const;
};

// An animation clip containing all bone channels
class AnimationClip {
public:
    AnimationClip() = default;
    AnimationClip(const std::string& name, float duration, float ticksPerSecond);
    
    void AddChannel(AnimationChannel channel);
    const AnimationChannel* FindChannel(const std::string& boneName) const;
    
    const std::string& GetName() const { return name_; }
    float GetDuration() const { return duration_; }
    float GetTicksPerSecond() const { return ticksPerSecond_; }
    float GetDurationInSeconds() const { return duration_ / ticksPerSecond_; }
    const std::vector<AnimationChannel>& GetChannels() const { return channels_; }
    
private:
    std::string name_;
    float duration_ = 0.0f;
    float ticksPerSecond_ = 24.0f;
    std::vector<AnimationChannel> channels_;
    std::unordered_map<std::string, size_t> channelMap_;
};

}  // namespace se
