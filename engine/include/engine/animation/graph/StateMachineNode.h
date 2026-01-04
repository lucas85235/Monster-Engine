#pragma once

#include "engine/animation/graph/IAnimationNode.h"
#include "engine/animation/advanced/Pose.h"

#include <functional>
#include <vector>
#include <unordered_map>

namespace se {
namespace anim {

class AnimationGraph;

struct StateTransition {
    std::string fromState;
    std::string toState;
    std::function<bool(const AnimationGraph&)> condition;
    float duration = 0.25f;
    bool hasExitTime = false;
    float exitTime = 1.0f;
};

struct AnimationState {
    std::string name;
    AnimationNodePtr node;
    float speed = 1.0f;
    bool loop = true;
    
    AnimationState() = default;
    AnimationState(const std::string& n, AnimationNodePtr nd, float spd = 1.0f, bool lp = true)
        : name(n), node(std::move(nd)), speed(spd), loop(lp) {}
};

class StateMachineNode : public IAnimationNode {
public:
    StateMachineNode() = default;
    explicit StateMachineNode(const std::string& name);
    
    void Evaluate(const AnimationContext& ctx, Pose& outPose) override;
    void Reset() override;
    std::string GetName() const override { return name_; }
    
    void AddState(AnimationState state);
    void RemoveState(const std::string& name);
    AnimationState* GetState(const std::string& name);
    const AnimationState* GetState(const std::string& name) const;
    
    void AddTransition(const StateTransition& transition);
    void AddTransition(const std::string& from, const std::string& to,
                       std::function<bool(const AnimationGraph&)> condition,
                       float duration = 0.25f);
    
    void SetDefaultState(const std::string& name);
    const std::string& GetDefaultState() const { return defaultState_; }
    
    void TransitionTo(const std::string& stateName, float duration = 0.25f);
    
    const std::string& GetCurrentState() const { return currentState_; }
    bool IsTransitioning() const { return isTransitioning_; }
    float GetTransitionProgress() const { return transitionProgress_; }
    
    const std::vector<AnimationState>& GetStates() const { return states_; }
    const std::vector<StateTransition>& GetTransitions() const { return transitions_; }
    
private:
    void CheckTransitions(const AnimationContext& ctx);
    void UpdateTransition(float dt);
    
    std::string name_ = "StateMachineNode";
    std::vector<AnimationState> states_;
    std::vector<StateTransition> transitions_;
    std::unordered_map<std::string, size_t> stateIndexMap_;
    
    std::string defaultState_;
    std::string currentState_;
    std::string targetState_;
    
    bool isTransitioning_ = false;
    float transitionDuration_ = 0.25f;
    float transitionProgress_ = 0.0f;
    
    Pose currentPose_;
    Pose targetPose_;
};

}  // namespace anim
}  // namespace se
