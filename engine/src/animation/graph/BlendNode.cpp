#include "engine/animation/graph/BlendNode.h"
#include "engine/animation/graph/AnimationGraph.h"

#include <algorithm>

namespace se {
namespace anim {

BlendNode::BlendNode(const std::string& name)
    : name_(name) {
}

void BlendNode::Evaluate(const AnimationContext& ctx, Pose& outPose) {
    // Get weight from graph parameter if bound
    float weight = blendWeight_;
    if (!blendParameter_.empty() && ctx.graph) {
        weight = ctx.graph->GetFloat(blendParameter_);
    }
    weight = std::clamp(weight, 0.0f, 1.0f);
    
    // If only one input, use it directly
    if (!inputA_ && !inputB_) {
        return;
    }
    
    if (!inputA_) {
        if (inputB_) {
            inputB_->Evaluate(ctx, outPose);
        }
        return;
    }
    
    if (!inputB_) {
        inputA_->Evaluate(ctx, outPose);
        return;
    }
    
    // Evaluate both inputs
    poseA_.Resize(outPose.GetBoneCount());
    poseB_.Resize(outPose.GetBoneCount());
    
    inputA_->Evaluate(ctx, poseA_);
    inputB_->Evaluate(ctx, poseB_);
    
    // Blend based on weight
    outPose = poseA_;
    outPose.BlendWith(poseB_, weight);
}

void BlendNode::Reset() {
    if (inputA_) inputA_->Reset();
    if (inputB_) inputB_->Reset();
}

void BlendNode::SetInputA(AnimationNodePtr node) {
    inputA_ = std::move(node);
}

void BlendNode::SetInputB(AnimationNodePtr node) {
    inputB_ = std::move(node);
}

void BlendNode::SetBlendWeight(float weight) {
    blendWeight_ = std::clamp(weight, 0.0f, 1.0f);
}

void BlendNode::SetBlendParameter(const std::string& paramName) {
    blendParameter_ = paramName;
}

}  // namespace anim
}  // namespace se
