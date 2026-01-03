#pragma once

#include "engine/navigation/AStar.h"
#include "engine/ecs/Entity.h"
#include "engine/ecs/SimpleComponents.h"

#include <glm.hpp>
#include <cstdint>
#include <functional>
#include <vector>

namespace se {
namespace nav {

using Vector3 = glm::vec3;

enum class AgentState {
    Idle,           // No target set
    RequestingPath, // Waiting for async path
    Moving,         // Following path
    Stuck,          // Can't reach target
    Arrived         // Reached destination
};

struct PathfindingAgentComponent {
    // Movement configuration
    float moveSpeed        = 5.0f;         // Units per second
    float turnSpeed        = 360.0f;       // Degrees per second
    float stoppingDistance = 0.5f;         // Stop this far from target
    float acceleration     = 10.0f;        // Speed change rate
    float waypointRadius   = 0.3f;         // Distance to consider waypoint reached

    // Repath settings
    float repathInterval   = 0.5f;         // Seconds between re-path checks
    float repathThreshold  = 5.0f;         // Target moved this much = repath (increased for stability)
    float minRepathInterval = 1.0f;        // Minimum seconds between path recalculations
    bool  autoRepath       = true;         // Automatically repath when target moves



    // Pathfinding settings
    PathfindingSettings pathSettings;

    // Current state (runtime)
    AgentState              state             = AgentState::Idle;
    std::vector<Vector3>    currentPath;
    int32_t                 currentWaypoint   = 0;
    float                   timeSinceRepath   = 0.0f;
    uint32_t                pendingRequestId  = 0;
    float                   currentSpeed      = 0.0f;

    // Path caching - prevents recalculating the same path
    Vector3 lastPathStart{0.0f};
    Vector3 lastPathGoal{0.0f};
    float   pathCacheRadius = 0.5f;  // Reuse path if start/goal within this radius

    // Path validity tracking
    bool    pathValid = false;
    float   pathAge = 0.0f;          // Time since path was calculated
    float   maxPathAge = 10.0f;      // Max age before path is considered stale

    // Explicit repath control
    bool    forceRepath = false;     // Manual trigger for repath

    // Stuck state cooldown - prevents spamming path requests on failure
    float   stuckCooldown = 0.0f;    // Current cooldown timer
    float   stuckRetryInterval = 1.0f; // Wait this long before retrying after failure

    // Target
    Vector3     targetPosition{0.0f};
    Entity      targetEntity;              // For following another entity
    bool        hasTarget      = false;
    bool        followEntity   = false;

    // Callbacks (optional)
    std::function<void()>         onPathFound;
    std::function<void()>         onPathFailed;
    std::function<void()>         onArrived;
    std::function<void(Vector3)>  onWaypointReached;

    // Methods
    void SetTarget(const Vector3& position) {
        targetPosition = position;
        hasTarget      = true;
        followEntity   = false;
        forceRepath    = true;  // Request new path without resetting state
    }

    void SetTarget(Entity entity) {
        targetEntity = entity;
        hasTarget    = true;
        followEntity = true;
        forceRepath  = true;  // Request new path without resetting state
    }

    void ClearTarget() {
        hasTarget    = false;
        followEntity = false;
        currentPath.clear();
        currentWaypoint = 0;
        pathValid    = false;
        forceRepath  = false;
        state        = AgentState::Idle;
    }

    void InvalidatePath() {
        pathValid = false;
        forceRepath = true;
    }

    bool HasPath() const {
        return !currentPath.empty();
    }

    bool IsMoving() const {
        return state == AgentState::Moving;
    }

    bool HasArrived() const {
        return state == AgentState::Arrived;
    }

    bool IsStuck() const {
        return state == AgentState::Stuck;
    }

    Vector3 GetCurrentWaypointPosition() const {
        if (currentPath.empty() || currentWaypoint >= static_cast<int32_t>(currentPath.size())) {
            return targetPosition;
        }
        return currentPath[currentWaypoint];
    }

    float GetRemainingDistance() const {
        if (currentPath.empty()) return 0.0f;
        float dist = 0.0f;
        for (size_t i = currentWaypoint; i < currentPath.size() - 1; ++i) {
            dist += glm::distance(currentPath[i], currentPath[i + 1]);
        }
        return dist;
    }

    bool NeedsRepath(const Vector3& currentPos) const;
    bool IsPathCacheValid(const Vector3& start, const Vector3& goal) const;
};

// Forward declarations
class NavigationSystem;

// Update function to be called from game logic or a system
void UpdatePathfindingAgent(PathfindingAgentComponent& agent,
                            TransformComponent& transform,
                            NavigationSystem& navSystem,
                            Entity entity,
                            float dt);

}  // namespace nav
}  // namespace se
