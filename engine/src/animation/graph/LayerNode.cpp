#include "engine/animation/graph/LayerNode.h"
#include "engine/animation/graph/AnimationGraph.h"

#include <algorithm>

namespace se {
namespace anim {

LayerNode::LayerNode(const std::string& name)
    : name_(name) {
}

void LayerNode::SetBaseNode(AnimationNodePtr node) {
    baseNode_ = std::move(node);
}

size_t LayerNode::AddLayer(LayerConfig config) {
    Layer layer;
    layer.name = config.name;
    layer.node = std::move(config.node);
    layer.mask = config.mask;
    layer.blendMode = config.blendMode;
    layer.weight = config.weight;
    layer.weightParameter = config.weightParameter;
    
    layers_.push_back(std::move(layer));
    return layers_.size() - 1;
}

void LayerNode::RemoveLayer(const std::string& name) {
    layers_.erase(
        std::remove_if(layers_.begin(), layers_.end(),
            [&name](const Layer& l) { return l.name == name; }),
        layers_.end()
    );
}

void LayerNode::SetLayerWeight(const std::string& name, float weight) {
    for (auto& layer : layers_) {
        if (layer.name == name) {
            layer.weight = std::clamp(weight, 0.0f, 1.0f);
            return;
        }
    }
}

void LayerNode::SetLayerWeight(size_t index, float weight) {
    if (index < layers_.size()) {
        layers_[index].weight = std::clamp(weight, 0.0f, 1.0f);
    }
}

float LayerNode::GetLayerWeight(const std::string& name) const {
    for (const auto& layer : layers_) {
        if (layer.name == name) {
            return layer.weight;
        }
    }
    return 0.0f;
}

float LayerNode::GetLayerWeight(size_t index) const {
    if (index < layers_.size()) {
        return layers_[index].weight;
    }
    return 0.0f;
}

void LayerNode::Evaluate(const AnimationContext& ctx, Pose& outPose) {
    // Evaluate base node first
    if (baseNode_) {
        baseNode_->Evaluate(ctx, basePose_);
        outPose = basePose_;
    } else {
        outPose.SetIdentity();
    }
    
    // Apply each layer
    for (auto& layer : layers_) {
        if (!layer.node) continue;
        
        // Get weight from parameter if bound
        float weight = layer.weight;
        if (!layer.weightParameter.empty() && ctx.graph) {
            weight = ctx.graph->GetFloat(layer.weightParameter);
        }
        
        if (weight <= 0.001f) continue;
        
        // Evaluate layer pose
        layer.pose.Resize(outPose.GetBoneCount());
        layer.node->Evaluate(ctx, layer.pose);
        
        // Apply layer based on blend mode and mask
        for (size_t i = 0; i < outPose.GetBoneCount(); ++i) {
            if (!layer.mask.Contains(static_cast<int>(i))) continue;
            
            float boneWeight = layer.mask.GetWeight(static_cast<int>(i)) * weight;
            if (boneWeight <= 0.001f) continue;
            
            switch (layer.blendMode) {
                case LayerBlendMode::Override:
                    outPose[i] = BoneTransform::Blend(outPose[i], layer.pose[i], boneWeight);
                    break;
                    
                case LayerBlendMode::Blend:
                    outPose[i] = BoneTransform::Blend(outPose[i], layer.pose[i], boneWeight);
                    break;
                    
                case LayerBlendMode::Additive:
                    outPose[i] = BoneTransform::BlendAdditive(outPose[i], layer.pose[i], boneWeight);
                    break;
            }
        }
    }
}

void LayerNode::Reset() {
    if (baseNode_) {
        baseNode_->Reset();
    }
    for (auto& layer : layers_) {
        if (layer.node) {
            layer.node->Reset();
        }
    }
}

}  // namespace anim
}  // namespace se
