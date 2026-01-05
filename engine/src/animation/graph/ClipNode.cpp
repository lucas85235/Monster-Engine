#include "engine/animation/graph/ClipNode.h"
#include "engine/animation/advanced/Pose.h"

namespace se {
namespace anim {

ClipNode::ClipNode(std::shared_ptr<AnimationClip> clip, bool loop)
    : clip_(clip), loop_(loop) {
}

ClipNode::ClipNode(const std::string& name, std::shared_ptr<AnimationClip> clip, bool loop)
    : name_(name), clip_(clip), loop_(loop) {
}

void ClipNode::Evaluate(const AnimationContext& ctx, Pose& outPose) {
    if (!clip_ || !ctx.skeleton) {
        return;
    }
    
    // Advance time
    float deltaTime = ctx.deltaTime * playbackSpeed_;
    currentTime_ += deltaTime;
    
    float duration = clip_->GetDurationInSeconds();
    if (duration <= 0.0f) {
        return;
    }
    
    // Handle looping
    if (currentTime_ >= duration) {
        if (loop_) {
            currentTime_ = std::fmod(currentTime_, duration);
            finished_ = false;
        } else {
            currentTime_ = duration;
            finished_ = true;
        }
    }
    
    // Sample pose from clip
    outPose.SetFromClip(clip_.get(), currentTime_, ctx.skeleton);
}

void ClipNode::Reset() {
    currentTime_ = 0.0f;
    finished_ = false;
}

float ClipNode::GetNormalizedTime() const {
    if (!clip_) return 0.0f;
    float duration = clip_->GetDurationInSeconds();
    if (duration <= 0.0f) return 0.0f;
    return currentTime_ / duration;
}

}  // namespace anim
}  // namespace se
