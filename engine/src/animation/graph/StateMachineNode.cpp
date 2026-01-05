#include "engine/animation/graph/StateMachineNode.h"
#include "engine/animation/graph/AnimationGraph.h"
#include "engine/Log.h"

#include <algorithm>

namespace se {
namespace anim {

StateMachineNode::StateMachineNode(const std::string& name)
    : name_(name) {
}

void StateMachineNode::AddState(AnimationState state) {
    stateIndexMap_[state.name] = states_.size();
    states_.push_back(std::move(state));
    
    if (defaultState_.empty()) {
        defaultState_ = states_.back().name;
        currentState_ = defaultState_;
    }
}

void StateMachineNode::RemoveState(const std::string& name) {
    auto it = stateIndexMap_.find(name);
    if (it == stateIndexMap_.end()) return;
    
    size_t index = it->second;
    states_.erase(states_.begin() + static_cast<int>(index));
    stateIndexMap_.erase(it);
    
    // Rebuild index map
    stateIndexMap_.clear();
    for (size_t i = 0; i < states_.size(); ++i) {
        stateIndexMap_[states_[i].name] = i;
    }
}

AnimationState* StateMachineNode::GetState(const std::string& name) {
    auto it = stateIndexMap_.find(name);
    if (it != stateIndexMap_.end()) {
        return &states_[it->second];
    }
    return nullptr;
}

const AnimationState* StateMachineNode::GetState(const std::string& name) const {
    auto it = stateIndexMap_.find(name);
    if (it != stateIndexMap_.end()) {
        return &states_[it->second];
    }
    return nullptr;
}

void StateMachineNode::AddTransition(const StateTransition& transition) {
    transitions_.push_back(transition);
}

void StateMachineNode::AddTransition(const std::string& from, const std::string& to,
                                     std::function<bool(const AnimationGraph&)> condition,
                                     float duration) {
    StateTransition t;
    t.fromState = from;
    t.toState = to;
    t.condition = condition;
    t.duration = duration;
    transitions_.push_back(t);
}

void StateMachineNode::SetDefaultState(const std::string& name) {
    defaultState_ = name;
    if (currentState_.empty()) {
        currentState_ = defaultState_;
    }
}

void StateMachineNode::TransitionTo(const std::string& stateName, float duration) {
    if (stateName == currentState_) return;
    if (!GetState(stateName)) return;
    
    targetState_ = stateName;
    transitionDuration_ = duration;
    transitionProgress_ = 0.0f;
    isTransitioning_ = true;
    
    // Reset target state's node
    auto* target = GetState(targetState_);
    if (target && target->node) {
        target->node->Reset();
    }
}

void StateMachineNode::Evaluate(const AnimationContext& ctx, Pose& outPose) {
    if (states_.empty()) return;
    
    // Initialize current state if needed
    if (currentState_.empty()) {
        currentState_ = defaultState_;
    }
    
    auto* currentStateData = GetState(currentState_);
    if (!currentStateData || !currentStateData->node) return;
    
    // Check for transitions
    if (!isTransitioning_) {
        CheckTransitions(ctx);
    }
    
    // Handle transition blending
    if (isTransitioning_) {
        UpdateTransition(ctx.deltaTime);
        
        auto* targetStateData = GetState(targetState_);
        if (targetStateData && targetStateData->node) {
            // Evaluate both states
            currentPose_.Resize(outPose.GetBoneCount());
            targetPose_.Resize(outPose.GetBoneCount());
            
            currentStateData->node->Evaluate(ctx, currentPose_);
            targetStateData->node->Evaluate(ctx, targetPose_);
            
            // Blend based on transition progress
            outPose = currentPose_;
            outPose.BlendWith(targetPose_, transitionProgress_);
        } else {
            currentStateData->node->Evaluate(ctx, outPose);
        }
    } else {
        currentStateData->node->Evaluate(ctx, outPose);
    }
}

void StateMachineNode::Reset() {
    currentState_ = defaultState_;
    targetState_.clear();
    isTransitioning_ = false;
    transitionProgress_ = 0.0f;
    
    for (auto& state : states_) {
        if (state.node) {
            state.node->Reset();
        }
    }
}

void StateMachineNode::CheckTransitions(const AnimationContext& ctx) {
    if (!ctx.graph) return;
    
    for (const auto& transition : transitions_) {
        if (transition.fromState != currentState_) continue;
        
        // Check exit time if required
        if (transition.hasExitTime) {
            auto* state = GetState(currentState_);
            if (state && state->node) {
                // TODO: Get normalized time from node
            }
        }
        
        // Evaluate condition
        if (transition.condition && transition.condition(*ctx.graph)) {
            TransitionTo(transition.toState, transition.duration);
            
            // Notify graph of state change
            if (ctx.graph) {
                const_cast<AnimationGraph*>(ctx.graph)->NotifyStateChanged(currentState_, transition.toState);
            }
            break;
        }
    }
}

void StateMachineNode::UpdateTransition(float dt) {
    if (!isTransitioning_) return;
    
    transitionProgress_ += dt / transitionDuration_;
    
    if (transitionProgress_ >= 1.0f) {
        transitionProgress_ = 1.0f;
        currentState_ = targetState_;
        targetState_.clear();
        isTransitioning_ = false;
    }
}

}  // namespace anim
}  // namespace se
