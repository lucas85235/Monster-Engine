#pragma once

#include "engine/ai/StateNode.h"
#include "engine/gameplay/AIController.h"
#include "engine/ecs/SimpleComponents.h"

#include <glm.hpp>

namespace se {
namespace ai {

// Flee state - runs away from target
class FleeState : public StateNode {
   public:
    FleeState() { SetName("Flee"); }

    void SetFleeDistance(float distance) { fleeDistance_ = distance; }
    void SetSafeDistance(float distance) { safeDistance_ = distance; }

   protected:
    void OnEnter(StateTreeContext& ctx) override {
        CalculateFleeDestination(ctx);
    }

    NodeStatus OnTick(StateTreeContext& ctx) override {
        if (!ctx.controller) {
            return NodeStatus::Failure;
        }

        // Check if safe
        float distance = ctx.blackboard->GetDistanceToTarget();
        if (distance >= safeDistance_) {
            return NodeStatus::Success;
        }

        // If reached flee point but not safe yet, recalculate
        if (ctx.controller->HasReachedDestination()) {
            CalculateFleeDestination(ctx);
        }

        return NodeStatus::Running;
    }

   private:
    void CalculateFleeDestination(StateTreeContext& ctx) {
        if (!ctx.controller) return;

        glm::vec3 selfPos   = ctx.blackboard->GetSelfPosition();
        glm::vec3 targetPos = ctx.blackboard->GetTargetPosition();

        // Calculate direction away from target
        glm::vec3 fleeDir = selfPos - targetPos;
        if (glm::length(fleeDir) > 0.01f) {
            fleeDir = glm::normalize(fleeDir);
        } else {
            fleeDir = glm::vec3{1.0f, 0.0f, 0.0f};  // Fallback
        }

        glm::vec3 fleePoint = selfPos + fleeDir * fleeDistance_;
        ctx.controller->MoveToLocation(fleePoint);
    }

    float fleeDistance_ = 10.0f;
    float safeDistance_ = 15.0f;
};

}  // namespace ai
}  // namespace se
