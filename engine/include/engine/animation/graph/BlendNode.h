#pragma once

#include "engine/animation/graph/IAnimationNode.h"
#include "engine/animation/advanced/Pose.h"

#include <string>

namespace se {
namespace anim {

class BlendNode : public IAnimationNode {
public:
    BlendNode() = default;
    explicit BlendNode(const std::string& name);
    
    void Evaluate(const AnimationContext& ctx, Pose& outPose) override;
    void Reset() override;
    std::string GetName() const override { return name_; }
    
    void SetInputA(AnimationNodePtr node);
    void SetInputB(AnimationNodePtr node);
    
    IAnimationNode* GetInputA() { return inputA_.get(); }
    IAnimationNode* GetInputB() { return inputB_.get(); }
    
    void SetBlendWeight(float weight);
    float GetBlendWeight() const { return blendWeight_; }
    
    // Bind to a graph parameter for automatic weight updates
    void SetBlendParameter(const std::string& paramName);
    const std::string& GetBlendParameter() const { return blendParameter_; }
    
private:
    std::string name_ = "BlendNode";
    AnimationNodePtr inputA_;
    AnimationNodePtr inputB_;
    float blendWeight_ = 0.0f;
    std::string blendParameter_;
    
    Pose poseA_;
    Pose poseB_;
};

}  // namespace anim
}  // namespace se
