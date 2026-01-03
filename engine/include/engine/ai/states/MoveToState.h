#pragma once

#include "engine/ai/StateNode.h"
#include "engine/gameplay/AIController.h"

#include <glm.hpp>

namespace se {
namespace ai {

// MoveToState - moves to a specific position
class MoveToState : public StateNode {
   public:
    MoveToState() { SetName("MoveTo"); }
    explicit MoveToState(const glm::vec3& destination) 
        : destination_(destination), useBlackboard_(false) {
        SetName("MoveTo");
    }

    void SetDestination(const glm::vec3& destination) {
        destination_    = destination;
        useBlackboard_  = false;
    }

    // Use blackboard key for destination
    void SetDestinationKey(const std::string& key) {
        destinationKey_ = key;
        useBlackboard_  = true;
    }

   protected:
    void OnEnter(StateTreeContext& ctx) override {
        if (!ctx.controller) return;

        glm::vec3 dest = destination_;
        if (useBlackboard_ && ctx.blackboard->Has(destinationKey_)) {
            dest = ctx.blackboard->Get<glm::vec3>(destinationKey_);
        }

        ctx.controller->MoveToLocation(dest);
    }

    NodeStatus OnTick(StateTreeContext& ctx) override {
        if (!ctx.controller) {
            return NodeStatus::Failure;
        }

        if (ctx.controller->HasReachedDestination()) {
            return NodeStatus::Success;
        }

        if (!ctx.controller->IsMoving() && !ctx.controller->IsPathPending()) {
            return NodeStatus::Failure;  // Path failed
        }

        return NodeStatus::Running;
    }

   private:
    glm::vec3   destination_{0.0f};
    std::string destinationKey_;
    bool        useBlackboard_ = false;
};

// ReturnHomeState - returns to home position stored in blackboard
class ReturnHomeState : public StateNode {
   public:
    ReturnHomeState() { SetName("ReturnHome"); }

   protected:
    void OnEnter(StateTreeContext& ctx) override {
        if (!ctx.controller) return;

        if (ctx.blackboard->Has(BlackboardKeys::HomePosition)) {
            glm::vec3 home = ctx.blackboard->Get<glm::vec3>(BlackboardKeys::HomePosition);
            ctx.controller->MoveToLocation(home);
        }
    }

    NodeStatus OnTick(StateTreeContext& ctx) override {
        if (!ctx.controller) {
            return NodeStatus::Failure;
        }

        if (ctx.controller->HasReachedDestination()) {
            return NodeStatus::Success;
        }

        return NodeStatus::Running;
    }
};

}  // namespace ai
}  // namespace se
