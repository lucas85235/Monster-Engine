#pragma once

#include "engine/ai/StateNode.h"
#include "engine/gameplay/AIController.h"

#include <glm.hpp>
#include <vector>

namespace se {
namespace ai {

// Patrol state - moves between waypoints
class PatrolState : public StateNode {
   public:
    PatrolState() { SetName("Patrol"); }

    void SetWaypoints(const std::vector<glm::vec3>& waypoints) {
        waypoints_ = waypoints;
    }

    void AddWaypoint(const glm::vec3& point) {
        waypoints_.push_back(point);
    }

    void SetWaitTime(float seconds) { waitTime_ = seconds; }
    void SetLoop(bool loop) { loop_ = loop; }

   protected:
    void OnEnter(StateTreeContext& ctx) override {
        if (waypoints_.empty()) return;

        // Get current patrol index from blackboard or start at 0
        currentIndex_ = ctx.blackboard->Get<int>(BlackboardKeys::PatrolIndex, 0);
        waitTimer_    = 0.0f;
        isWaiting_    = false;

        // Start moving to first waypoint
        if (ctx.controller) {
            ctx.controller->MoveToLocation(waypoints_[currentIndex_]);
        }
    }

    NodeStatus OnTick(StateTreeContext& ctx) override {
        if (waypoints_.empty()) {
            return NodeStatus::Failure;
        }

        if (!ctx.controller) {
            return NodeStatus::Failure;
        }

        // If waiting at waypoint
        if (isWaiting_) {
            waitTimer_ += ctx.deltaTime;
            if (waitTimer_ >= waitTime_) {
                isWaiting_ = false;
                MoveToNextWaypoint(ctx);
            }
            return NodeStatus::Running;
        }

        // Check if reached waypoint
        if (ctx.controller->HasReachedDestination()) {
            if (waitTime_ > 0.0f) {
                isWaiting_  = true;
                waitTimer_  = 0.0f;
            } else {
                MoveToNextWaypoint(ctx);
            }
        }

        return NodeStatus::Running;
    }

    void OnExit(StateTreeContext& ctx) override {
        // Save patrol index for later
        ctx.blackboard->Set(BlackboardKeys::PatrolIndex, currentIndex_);
    }

   private:
    void MoveToNextWaypoint(StateTreeContext& ctx) {
        currentIndex_++;

        if (currentIndex_ >= static_cast<int>(waypoints_.size())) {
            if (loop_) {
                currentIndex_ = 0;
            } else {
                return;  // Patrol complete
            }
        }

        if (ctx.controller) {
            ctx.controller->MoveToLocation(waypoints_[currentIndex_]);
        }
    }

    std::vector<glm::vec3> waypoints_;
    int                    currentIndex_ = 0;
    float                  waitTime_     = 1.0f;
    float                  waitTimer_    = 0.0f;
    bool                   isWaiting_    = false;
    bool                   loop_         = true;
};

}  // namespace ai
}  // namespace se
