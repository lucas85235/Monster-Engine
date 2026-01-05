#include "engine/animation/graph/BlendSpaceNode.h"
#include "engine/animation/graph/AnimationGraph.h"

namespace se {
namespace anim {

// BlendSpace1DNode

BlendSpace1DNode::BlendSpace1DNode(const std::string& name)
    : name_(name) {
}

BlendSpace1DNode::BlendSpace1DNode(const std::string& name, std::unique_ptr<BlendSpace1D> blendSpace)
    : name_(name), blendSpace_(std::move(blendSpace)) {
}

void BlendSpace1DNode::Evaluate(const AnimationContext& ctx, Pose& outPose) {
    if (!blendSpace_ || !ctx.skeleton) {
        return;
    }
    
    // Get parameter from binding if set
    float param = parameter_;
    if (!parameterBinding_.empty() && ctx.graph) {
        param = ctx.graph->GetFloat(parameterBinding_);
    }
    
    // Advance animation time
    animationTime_ += ctx.deltaTime * playbackSpeed_;
    
    // Evaluate blend space
    blendSpace_->Evaluate(param, outPose, animationTime_, ctx.skeleton);
}

void BlendSpace1DNode::Reset() {
    animationTime_ = 0.0f;
}

void BlendSpace1DNode::SetBlendSpace(std::unique_ptr<BlendSpace1D> blendSpace) {
    blendSpace_ = std::move(blendSpace);
}

void BlendSpace1DNode::SetParameterBinding(const std::string& paramName) {
    parameterBinding_ = paramName;
}

// BlendSpace2DNode

BlendSpace2DNode::BlendSpace2DNode(const std::string& name)
    : name_(name) {
}

BlendSpace2DNode::BlendSpace2DNode(const std::string& name, std::unique_ptr<BlendSpace2D> blendSpace)
    : name_(name), blendSpace_(std::move(blendSpace)) {
}

void BlendSpace2DNode::Evaluate(const AnimationContext& ctx, Pose& outPose) {
    if (!blendSpace_ || !ctx.skeleton) {
        return;
    }
    
    // Get parameters from bindings if set
    glm::vec2 param = parameter_;
    if (!parameterBindingX_.empty() && ctx.graph) {
        param.x = ctx.graph->GetFloat(parameterBindingX_);
    }
    if (!parameterBindingY_.empty() && ctx.graph) {
        param.y = ctx.graph->GetFloat(parameterBindingY_);
    }
    
    // Advance animation time
    animationTime_ += ctx.deltaTime * playbackSpeed_;
    
    // Evaluate blend space
    blendSpace_->Evaluate(param, outPose, animationTime_, ctx.skeleton);
}

void BlendSpace2DNode::Reset() {
    animationTime_ = 0.0f;
}

void BlendSpace2DNode::SetBlendSpace(std::unique_ptr<BlendSpace2D> blendSpace) {
    blendSpace_ = std::move(blendSpace);
}

void BlendSpace2DNode::SetParameterBindingX(const std::string& paramName) {
    parameterBindingX_ = paramName;
}

void BlendSpace2DNode::SetParameterBindingY(const std::string& paramName) {
    parameterBindingY_ = paramName;
}

}  // namespace anim
}  // namespace se
