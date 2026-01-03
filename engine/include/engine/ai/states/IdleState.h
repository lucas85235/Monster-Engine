#pragma once

#include "engine/ai/StateNode.h"

namespace se {
namespace ai {

// Idle state - does nothing, useful as starting point
class IdleState : public StateNode {
   public:
    IdleState() { SetName("Idle"); }

   protected:
    NodeStatus OnTick(StateTreeContext& ctx) override {
        return NodeStatus::Running;
    }
};

// Wait state - waits for a duration then succeeds
class WaitState : public StateNode {
   public:
    explicit WaitState(float duration = 1.0f) : duration_(duration) {
        SetName("Wait");
    }

   protected:
    NodeStatus OnTick(StateTreeContext& ctx) override {
        if (ctx.stateTime >= duration_) {
            return NodeStatus::Success;
        }
        return NodeStatus::Running;
    }

   private:
    float duration_;
};

}  // namespace ai
}  // namespace se
