#pragma once

#include "engine/animation/graph/IAnimationNode.h"
#include "engine/animation/advanced/AnimationLayer.h"
#include "engine/animation/advanced/BoneMask.h"

namespace se {
namespace anim {

struct LayerConfig {
    std::string name;
    AnimationNodePtr node;
    BoneMask mask;
    LayerBlendMode blendMode = LayerBlendMode::Blend;
    float weight = 1.0f;
    std::string weightParameter;  // Optional: bind weight to graph parameter
};

class LayerNode : public IAnimationNode {
public:
    LayerNode() = default;
    explicit LayerNode(const std::string& name);
    
    void Evaluate(const AnimationContext& ctx, Pose& outPose) override;
    void Reset() override;
    std::string GetName() const override { return name_; }
    
    void SetBaseNode(AnimationNodePtr node);
    IAnimationNode* GetBaseNode() { return baseNode_.get(); }
    
    size_t AddLayer(LayerConfig config);
    void RemoveLayer(const std::string& name);
    
    void SetLayerWeight(const std::string& name, float weight);
    void SetLayerWeight(size_t index, float weight);
    float GetLayerWeight(const std::string& name) const;
    float GetLayerWeight(size_t index) const;
    
    size_t GetLayerCount() const { return layers_.size(); }
    
private:
    std::string name_ = "LayerNode";
    AnimationNodePtr baseNode_;
    
    struct Layer {
        std::string name;
        AnimationNodePtr node;
        BoneMask mask;
        LayerBlendMode blendMode;
        float weight;
        std::string weightParameter;
        Pose pose;
    };
    
    std::vector<Layer> layers_;
    Pose basePose_;
};

}  // namespace anim
}  // namespace se
