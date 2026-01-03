#pragma once

#include "engine/ai/StateNode.h"
#include "engine/gameplay/AIController.h"
#include "engine/ecs/SimpleComponents.h"

#include <glm.hpp>

namespace se {
namespace ai {

// Chase state - follows target entity or position
class ChaseState : public StateNode {
   public:
    ChaseState() { SetName("Chase"); }

    void SetUpdateInterval(float seconds) { updateInterval_ = seconds; }
    void SetStopDistance(float distance) { stopDistance_ = distance; }

   protected:
    void OnEnter(StateTreeContext& ctx) override {
        updateTimer_ = 0.0f;
        UpdateTargetPosition(ctx);
    }

    NodeStatus OnTick(StateTreeContext& ctx) override {
        if (!ctx.controller) {
            return NodeStatus::Failure;
        }

        // Periodically update target position (for moving targets)
        updateTimer_ += ctx.deltaTime;
        if (updateTimer_ >= updateInterval_) {
            updateTimer_ = 0.0f;
            UpdateTargetPosition(ctx);
        }

        // Check if close enough
        float distance = ctx.blackboard->GetDistanceToTarget();
        if (distance <= stopDistance_) {
            ctx.controller->StopMovement();
            return NodeStatus::Success;
        }

        return NodeStatus::Running;
    }

    void OnExit(StateTreeContext& ctx) override {
        if (ctx.controller) {
            ctx.controller->StopMovement();
        }
    }

   private:
    void UpdateTargetPosition(StateTreeContext& ctx) {
        if (!ctx.controller) return;

        // Try to get target entity first
        Entity target = ctx.blackboard->GetTarget();
        if (target.IsValid() && target.HasComponent<TransformComponent>()) {
            glm::vec3 targetPos = target.GetComponent<TransformComponent>().Position;
            ctx.blackboard->SetTargetPosition(targetPos);
            ctx.controller->MoveToEntity(target);
        } else if (ctx.blackboard->Has(BlackboardKeys::TargetPosition)) {
            // Fall back to stored position
            glm::vec3 targetPos = ctx.blackboard->GetTargetPosition();
            ctx.controller->MoveToLocation(targetPos);
        }
    }

    float updateInterval_ = 0.2f;
    float stopDistance_   = 1.5f;
    float updateTimer_    = 0.0f;
};

}  // namespace ai
}  // namespace se
