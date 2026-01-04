#pragma once

#include <string>
#include <memory>

namespace se {

class SkinnedModelData;

namespace anim {

class Pose;
class AnimationGraph;

struct AnimationContext {
    const SkinnedModelData* skeleton = nullptr;
    const AnimationGraph* graph = nullptr;
    float deltaTime = 0.0f;
    float totalTime = 0.0f;
};

class IAnimationNode {
public:
    virtual ~IAnimationNode() = default;
    
    virtual void Evaluate(const AnimationContext& ctx, Pose& outPose) = 0;
    
    virtual void Reset() = 0;
    
    virtual std::string GetName() const = 0;
    
    virtual void SetPlaybackSpeed(float speed) { playbackSpeed_ = speed; }
    virtual float GetPlaybackSpeed() const { return playbackSpeed_; }
    
protected:
    float playbackSpeed_ = 1.0f;
};

using AnimationNodePtr = std::unique_ptr<IAnimationNode>;

}  // namespace anim
}  // namespace se
