#pragma once

#include "engine/ai/StateTreeNode.h"
#include <functional>
#include <memory>
#include <vector>

namespace se {

class StateNode;

// Condition function type
using ConditionFunc = std::function<bool(const StateTreeContext&)>;

// Transition between states
class Transition {
   public:
    Transition(StateNode* target) : target_(target) {}

    // Add condition using lambda
    Transition& When(ConditionFunc condition) {
        conditions_.push_back(std::move(condition));
        return *this;
    }

    // Evaluate all conditions
    bool Evaluate(const StateTreeContext& ctx) const {
        for (const auto& condition : conditions_) {
            if (!condition(ctx)) {
                return false;
            }
        }
        return !conditions_.empty();
    }

    StateNode* GetTarget() const { return target_; }

   private:
    StateNode*                  target_ = nullptr;
    std::vector<ConditionFunc>  conditions_;
};

// State node with enter/tick/exit and transitions
class StateNode : public StateTreeNode {
   public:
    StateNode() = default;
    ~StateNode() override = default;

    void Enter(StateTreeContext& ctx) override {
        OnEnter(ctx);
    }

    NodeStatus Tick(StateTreeContext& ctx) override {
        return OnTick(ctx);
    }

    void Exit(StateTreeContext& ctx) override {
        OnExit(ctx);
    }

    // Add transition to another state
    Transition& AddTransition(StateNode* target) {
        transitions_.emplace_back(target);
        return transitions_.back();
    }

    // Check transitions and return next state if any
    StateNode* EvaluateTransitions(const StateTreeContext& ctx) {
        for (auto& transition : transitions_) {
            if (transition.Evaluate(ctx)) {
                return transition.GetTarget();
            }
        }
        return nullptr;
    }

   protected:
    // Override these in derived classes
    virtual void OnEnter(StateTreeContext& ctx) {}
    virtual NodeStatus OnTick(StateTreeContext& ctx) { return NodeStatus::Running; }
    virtual void OnExit(StateTreeContext& ctx) {}

   private:
    std::vector<Transition> transitions_;
};

}  // namespace se
