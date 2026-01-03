#pragma once
/**
 * AnimationLayer.h - Multi-layer animation blending system.
 * 
 * Supports Override, Blend, and Additive blend modes with bone masks
 * for partial body animation.
 */

#include "engine/animation/advanced/Pose.h"
#include "engine/animation/advanced/BoneMask.h"

#include <string>
#include <vector>
#include <memory>

namespace se {
namespace anim {

enum class LayerBlendMode {
    Override,   // Replace base pose entirely
    Blend,      // Linear interpolation with base
    Additive    // Add delta on top of base
};

struct AnimationLayerData {
    std::string name;
    int priority = 0;
    LayerBlendMode blendMode = LayerBlendMode::Blend;
    std::string boneMaskName;
    float defaultWeight = 1.0f;
};

class AnimationLayer {
public:
    AnimationLayer() = default;
    AnimationLayer(const std::string& name, int priority, LayerBlendMode blendMode);
    
    void SetName(const std::string& name) { name_ = name; }
    const std::string& GetName() const { return name_; }
    
    void SetPriority(int priority) { priority_ = priority; }
    int GetPriority() const { return priority_; }
    
    void SetWeight(float weight) { weight_ = glm::clamp(weight, 0.0f, 1.0f); }
    float GetWeight() const { return weight_; }
    
    void SetBlendMode(LayerBlendMode mode) { blendMode_ = mode; }
    LayerBlendMode GetBlendMode() const { return blendMode_; }
    
    void SetBoneMask(const BoneMask& mask) { mask_ = mask; }
    const BoneMask& GetBoneMask() const { return mask_; }
    BoneMask& GetBoneMask() { return mask_; }
    
    void SetPose(const Pose& pose) { currentPose_ = pose; }
    const Pose& GetPose() const { return currentPose_; }
    
    void ApplyTo(Pose& basePose) const;
    
    bool IsActive() const { return weight_ > 0.001f; }
    
private:
    std::string name_;
    int priority_ = 0;
    float weight_ = 1.0f;
    LayerBlendMode blendMode_ = LayerBlendMode::Blend;
    BoneMask mask_;
    Pose currentPose_;
};

class AnimationLayerStack {
public:
    AnimationLayerStack() = default;
    
    AnimationLayer& AddLayer(const std::string& name, int priority, LayerBlendMode mode);
    
    AnimationLayer* GetLayer(const std::string& name);
    const AnimationLayer* GetLayer(const std::string& name) const;
    
    AnimationLayer* GetLayerByIndex(size_t index);
    const AnimationLayer* GetLayerByIndex(size_t index) const;
    
    void RemoveLayer(const std::string& name);
    void Clear();
    
    void Evaluate(Pose& outPose);
    
    size_t GetLayerCount() const { return layers_.size(); }
    
    void SetLayerWeight(const std::string& name, float weight);
    void SetLayerPose(const std::string& name, const Pose& pose);
    
private:
    void SortByPriority();
    
    std::vector<AnimationLayer> layers_;
    bool needsSort_ = false;
};

}  // namespace anim
}  // namespace se
