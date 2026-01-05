#pragma once

#include "engine/animation/graph/IAnimationNode.h"
#include "engine/animation/advanced/Pose.h"

#include <unordered_map>
#include <variant>
#include <string>
#include <functional>

namespace se {

class SkinnedModelData;

namespace anim {

using ParameterValue = std::variant<bool, float, int>;

class AnimationGraph {
public:
    AnimationGraph() = default;
    explicit AnimationGraph(const SkinnedModelData* skeleton);
    
    void SetSkeleton(const SkinnedModelData* skeleton) { skeleton_ = skeleton; }
    const SkinnedModelData* GetSkeleton() const { return skeleton_; }
    
    void SetRootNode(AnimationNodePtr node);
    IAnimationNode* GetRootNode() { return rootNode_.get(); }
    const IAnimationNode* GetRootNode() const { return rootNode_.get(); }
    
    void Update(float deltaTime);
    
    void Evaluate(Pose& outPose);
    
    // Parameter management
    void SetFloat(const std::string& name, float value);
    void SetBool(const std::string& name, bool value);
    void SetInt(const std::string& name, int value);
    void SetTrigger(const std::string& name);
    void ResetTrigger(const std::string& name);
    
    float GetFloat(const std::string& name) const;
    bool GetBool(const std::string& name) const;
    int GetInt(const std::string& name) const;
    bool HasParameter(const std::string& name) const;
    
    // Event callbacks
    using StateChangedCallback = std::function<void(const std::string& from, const std::string& to)>;
    void OnStateChanged(StateChangedCallback callback) { stateChangedCallback_ = callback; }
    void NotifyStateChanged(const std::string& from, const std::string& to);
    
    float GetTotalTime() const { return totalTime_; }
    float GetDeltaTime() const { return deltaTime_; }
    
private:
    const SkinnedModelData* skeleton_ = nullptr;
    AnimationNodePtr rootNode_;
    
    std::unordered_map<std::string, ParameterValue> parameters_;
    float totalTime_ = 0.0f;
    float deltaTime_ = 0.0f;
    
    StateChangedCallback stateChangedCallback_;
};

}  // namespace anim
}  // namespace se
