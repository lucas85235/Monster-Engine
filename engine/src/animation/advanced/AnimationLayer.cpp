#include "engine/animation/advanced/AnimationLayer.h"

#include "engine/Log.h"

#include <algorithm>

namespace se::anim {

AnimationLayer::AnimationLayer(const std::string& name, int priority, LayerBlendMode blendMode)
    : name_(name)
    , priority_(priority)
    , blendMode_(blendMode) {
}

void AnimationLayer::ApplyTo(Pose& basePose) const {
    if (!IsActive() || currentPose_.IsEmpty()) {
        return;
    }
    
    if (basePose.GetBoneCount() != currentPose_.GetBoneCount()) {
        SE_LOG_WARN("[AnimationLayer] Pose size mismatch in layer '{}'", name_);
        return;
    }
    
    bool hasMask = mask_.CountIncluded() > 0;
    
    for (size_t i = 0; i < basePose.GetBoneCount(); ++i) {
        // Check bone mask
        float boneWeight = weight_;
        if (hasMask) {
            if (!mask_.Contains(static_cast<int>(i))) {
                continue;  // Skip this bone
            }
            boneWeight *= mask_.GetWeight(static_cast<int>(i));
        }
        
        if (boneWeight < 0.001f) {
            continue;
        }
        
        switch (blendMode_) {
            case LayerBlendMode::Override:
                if (boneWeight >= 0.999f) {
                    basePose[i] = currentPose_[i];
                } else {
                    basePose[i] = BoneTransform::Blend(basePose[i], currentPose_[i], boneWeight);
                }
                break;
                
            case LayerBlendMode::Blend:
                basePose[i] = BoneTransform::Blend(basePose[i], currentPose_[i], boneWeight);
                break;
                
            case LayerBlendMode::Additive:
                basePose[i] = BoneTransform::BlendAdditive(basePose[i], currentPose_[i], boneWeight);
                break;
        }
    }
}

// ==================== AnimationLayerStack ====================

AnimationLayer& AnimationLayerStack::AddLayer(const std::string& name, int priority, LayerBlendMode mode) {
    // Check if layer already exists
    for (auto& layer : layers_) {
        if (layer.GetName() == name) {
            SE_LOG_WARN("[AnimationLayerStack] Layer '{}' already exists, updating", name);
            layer.SetPriority(priority);
            layer.SetBlendMode(mode);
            needsSort_ = true;
            return layer;
        }
    }
    
    layers_.emplace_back(name, priority, mode);
    needsSort_ = true;
    
    SE_LOG_INFO("[AnimationLayerStack] Added layer '{}' with priority {}", name, priority);
    return layers_.back();
}

AnimationLayer* AnimationLayerStack::GetLayer(const std::string& name) {
    for (auto& layer : layers_) {
        if (layer.GetName() == name) {
            return &layer;
        }
    }
    return nullptr;
}

const AnimationLayer* AnimationLayerStack::GetLayer(const std::string& name) const {
    for (const auto& layer : layers_) {
        if (layer.GetName() == name) {
            return &layer;
        }
    }
    return nullptr;
}

AnimationLayer* AnimationLayerStack::GetLayerByIndex(size_t index) {
    if (index >= layers_.size()) {
        return nullptr;
    }
    return &layers_[index];
}

const AnimationLayer* AnimationLayerStack::GetLayerByIndex(size_t index) const {
    if (index >= layers_.size()) {
        return nullptr;
    }
    return &layers_[index];
}

void AnimationLayerStack::RemoveLayer(const std::string& name) {
    auto it = std::remove_if(layers_.begin(), layers_.end(),
        [&name](const AnimationLayer& layer) {
            return layer.GetName() == name;
        });
    
    if (it != layers_.end()) {
        layers_.erase(it, layers_.end());
        SE_LOG_INFO("[AnimationLayerStack] Removed layer '{}'", name);
    }
}

void AnimationLayerStack::Clear() {
    layers_.clear();
}

void AnimationLayerStack::SortByPriority() {
    if (!needsSort_) {
        return;
    }
    
    std::sort(layers_.begin(), layers_.end(),
        [](const AnimationLayer& a, const AnimationLayer& b) {
            return a.GetPriority() < b.GetPriority();
        });
    
    needsSort_ = false;
}

void AnimationLayerStack::Evaluate(Pose& outPose) {
    SortByPriority();
    
    // Apply layers in priority order (lowest first = base)
    for (const auto& layer : layers_) {
        if (layer.IsActive()) {
            layer.ApplyTo(outPose);
        }
    }
}

void AnimationLayerStack::SetLayerWeight(const std::string& name, float weight) {
    if (auto* layer = GetLayer(name)) {
        layer->SetWeight(weight);
    }
}

void AnimationLayerStack::SetLayerPose(const std::string& name, const Pose& pose) {
    if (auto* layer = GetLayer(name)) {
        layer->SetPose(pose);
    }
}

}  // namespace se::anim
