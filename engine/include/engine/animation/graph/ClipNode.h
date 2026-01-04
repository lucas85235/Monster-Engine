#pragma once

#include "engine/animation/graph/IAnimationNode.h"
#include "engine/animation/AnimationClip.h"

#include <memory>

namespace se {
namespace anim {

class ClipNode : public IAnimationNode {
public:
    ClipNode() = default;
    explicit ClipNode(std::shared_ptr<AnimationClip> clip, bool loop = true);
    ClipNode(const std::string& name, std::shared_ptr<AnimationClip> clip, bool loop = true);
    
    void Evaluate(const AnimationContext& ctx, Pose& outPose) override;
    void Reset() override;
    std::string GetName() const override { return name_; }
    
    void SetClip(std::shared_ptr<AnimationClip> clip) { clip_ = clip; }
    std::shared_ptr<AnimationClip> GetClip() const { return clip_; }
    
    void SetLoop(bool loop) { loop_ = loop; }
    bool IsLooping() const { return loop_; }
    
    float GetCurrentTime() const { return currentTime_; }
    float GetNormalizedTime() const;
    bool HasFinished() const { return finished_; }
    
private:
    std::string name_ = "ClipNode";
    std::shared_ptr<AnimationClip> clip_;
    float currentTime_ = 0.0f;
    bool loop_ = true;
    bool finished_ = false;
};

}  // namespace anim
}  // namespace se
