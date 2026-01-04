#pragma once

#include "engine/animation/graph/IAnimationNode.h"
#include "engine/animation/advanced/BlendSpace.h"

#include <memory>

namespace se {
namespace anim {

class BlendSpace1DNode : public IAnimationNode {
public:
    BlendSpace1DNode() = default;
    explicit BlendSpace1DNode(const std::string& name);
    BlendSpace1DNode(const std::string& name, std::unique_ptr<BlendSpace1D> blendSpace);
    
    void Evaluate(const AnimationContext& ctx, Pose& outPose) override;
    void Reset() override;
    std::string GetName() const override { return name_; }
    
    void SetBlendSpace(std::unique_ptr<BlendSpace1D> blendSpace);
    BlendSpace1D* GetBlendSpace() { return blendSpace_.get(); }
    const BlendSpace1D* GetBlendSpace() const { return blendSpace_.get(); }
    
    void SetParameter(float value) { parameter_ = value; }
    float GetParameter() const { return parameter_; }
    
    // Bind to graph parameter
    void SetParameterBinding(const std::string& paramName);
    const std::string& GetParameterBinding() const { return parameterBinding_; }
    
private:
    std::string name_ = "BlendSpace1DNode";
    std::unique_ptr<BlendSpace1D> blendSpace_;
    float parameter_ = 0.0f;
    float animationTime_ = 0.0f;
    std::string parameterBinding_;
};

class BlendSpace2DNode : public IAnimationNode {
public:
    BlendSpace2DNode() = default;
    explicit BlendSpace2DNode(const std::string& name);
    BlendSpace2DNode(const std::string& name, std::unique_ptr<BlendSpace2D> blendSpace);
    
    void Evaluate(const AnimationContext& ctx, Pose& outPose) override;
    void Reset() override;
    std::string GetName() const override { return name_; }
    
    void SetBlendSpace(std::unique_ptr<BlendSpace2D> blendSpace);
    BlendSpace2D* GetBlendSpace() { return blendSpace_.get(); }
    const BlendSpace2D* GetBlendSpace() const { return blendSpace_.get(); }
    
    void SetParameter(const glm::vec2& value) { parameter_ = value; }
    glm::vec2 GetParameter() const { return parameter_; }
    
    // Bind X and Y to separate graph parameters
    void SetParameterBindingX(const std::string& paramName);
    void SetParameterBindingY(const std::string& paramName);
    const std::string& GetParameterBindingX() const { return parameterBindingX_; }
    const std::string& GetParameterBindingY() const { return parameterBindingY_; }
    
private:
    std::string name_ = "BlendSpace2DNode";
    std::unique_ptr<BlendSpace2D> blendSpace_;
    glm::vec2 parameter_{0.0f};
    float animationTime_ = 0.0f;
    std::string parameterBindingX_;
    std::string parameterBindingY_;
};

}  // namespace anim
}  // namespace se
