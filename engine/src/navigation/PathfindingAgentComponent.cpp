#include "engine/navigation/PathfindingAgentComponent.h"
#include "engine/navigation/NavigationSystem.h"
#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/Log.h"

#include <cmath>

namespace se {
namespace nav {

bool PathfindingAgentComponent::NeedsRepath(const Vector3& currentPos) const {
    // Forced repath via explicit request
    if (forceRepath) return true;
    
    // No valid path exists
    if (!pathValid || currentPath.empty()) return true;
    
    // State is Idle means we need a path
    if (state == AgentState::Idle && hasTarget) return true;
    
    // MINIMUM INTERVAL - don't repath too frequently
    if (timeSinceRepath < minRepathInterval) return false;
    
    // NEVER repath while moving successfully
    if (state == AgentState::Moving) {
        // Only repath if path is extremely old
        if (pathAge > maxPathAge) return true;
        return false;
    }
    
    // Arrived state - only repath if target moved significantly
    if (state == AgentState::Arrived) {
        float targetDrift = glm::distance(lastPathGoal, targetPosition);
        return targetDrift > repathThreshold;
    }
    
    // Stuck state handled separately with cooldown
    if (state == AgentState::Stuck) {
        return stuckCooldown <= 0.0f;
    }
    
    return false;
}




bool PathfindingAgentComponent::IsPathCacheValid(const Vector3& start, const Vector3& goal) const {
    if (!pathValid || currentPath.empty()) return false;
    
    float startDist = glm::distance(start, lastPathStart);
    float goalDist = glm::distance(goal, lastPathGoal);
    
    return startDist <= pathCacheRadius && goalDist <= pathCacheRadius;
}

void UpdatePathfindingAgent(PathfindingAgentComponent& agent, 
                            TransformComponent& transform,
                            NavigationSystem& navSystem,
                            Entity entity,
                            float dt) {
    // No target = stay idle
    if (!agent.hasTarget) {
        agent.state = AgentState::Idle;
        agent.currentSpeed = 0.0f;
        agent.pathValid = false;
        return;
    }
    
    Vector3 currentPos = transform.Position;
    
    // Update target position if following entity (without triggering repath immediately!)
    if (agent.followEntity && agent.targetEntity.IsValid()) {
        if (agent.targetEntity.HasComponent<TransformComponent>()) {
            agent.targetPosition = agent.targetEntity.GetComponent<TransformComponent>().Position;
        }
    }
    
    // Track path age
    agent.pathAge += dt;
    agent.timeSinceRepath += dt;
    
    // Handle stuck cooldown - prevents spamming on failure
    if (agent.state == AgentState::Stuck) {
        agent.stuckCooldown -= dt;
        if (agent.stuckCooldown > 0.0f) {
            // Still in cooldown, don't try to repath yet
            return;
        }
        // Cooldown expired, allow retry
        agent.stuckCooldown = 0.0f;
    }
    
    // Determine if we need a new path
    bool needsNewPath = agent.NeedsRepath(currentPos);
    
    // Request new path if needed and not already requesting
    if (needsNewPath && agent.state != AgentState::RequestingPath) {
        agent.forceRepath = false;
        
        SE_LOG_DEBUG("[PathfindingAgent] Requesting path: ({:.1f},{:.1f},{:.1f}) -> ({:.1f},{:.1f},{:.1f})",
                     currentPos.x, currentPos.y, currentPos.z,
                     agent.targetPosition.x, agent.targetPosition.y, agent.targetPosition.z);
        
        // Capture current position and target for the callback
        Vector3 requestStart = currentPos;
        Vector3 requestGoal = agent.targetPosition;
        
        // Request new path - callback handles completion
        agent.pendingRequestId = navSystem.RequestPath(
            currentPos,
            agent.targetPosition,
            [&agent, requestStart, requestGoal](const PathResult& result) {
                if (result.success && !result.path.empty()) {
                    agent.currentPath = result.path;
                    agent.currentWaypoint = 0;
                    
                    // Skip first waypoint if we're already at/past it
                    // This prevents oscillation when path starts at current cell center
                    if (agent.currentPath.size() > 1) {
                        Vector3 firstWP = agent.currentPath[0];
                        float distToFirst = glm::distance(
                            Vector3(requestStart.x, 0, requestStart.z),
                            Vector3(firstWP.x, 0, firstWP.z)
                        );
                        if (distToFirst < 0.5f) {  // Already at first waypoint
                            agent.currentWaypoint = 1;
                        }
                    }
                    
                    agent.state = AgentState::Moving;
                    agent.pathValid = true;
                    agent.pathAge = 0.0f;
                    agent.stuckCooldown = 0.0f;
                    
                    // Update cache with actual request params
                    agent.lastPathStart = requestStart;
                    agent.lastPathGoal = requestGoal;
                    
                    SE_LOG_DEBUG("[PathfindingAgent] Path found with {} waypoints, starting at wp {}", 
                                 result.path.size(), agent.currentWaypoint);
                    if (agent.onPathFound) agent.onPathFound();
                } else {
                    agent.state = AgentState::Stuck;
                    agent.pathValid = false;
                    agent.stuckCooldown = agent.stuckRetryInterval;
                    SE_LOG_WARN("[PathfindingAgent] Path request failed, retry in {:.1f}s", agent.stuckRetryInterval);
                    if (agent.onPathFailed) agent.onPathFailed();
                }
            },
            agent.pathSettings
        );
        
        agent.state = AgentState::RequestingPath;
        agent.timeSinceRepath = 0.0f;
        return;
    }


    
    // Wait for path request to complete via callback
    if (agent.state == AgentState::RequestingPath) {
        return;
    }
    
    // Move along path
    if (agent.state == AgentState::Moving && agent.pathValid && !agent.currentPath.empty()) {
        // Determine if we're on the last waypoint
        bool onLastWaypoint = (agent.currentWaypoint >= static_cast<int32_t>(agent.currentPath.size()) - 1);
        
        // On last waypoint, move toward actual target, not cell center
        Vector3 targetWaypoint = onLastWaypoint ? agent.targetPosition : agent.GetCurrentWaypointPosition();

        // Check distance to waypoint/target (2D)
        float distToWaypoint = glm::distance(
            Vector3(currentPos.x, 0, currentPos.z),
            Vector3(targetWaypoint.x, 0, targetWaypoint.z)
        );

        // Check if close enough to current waypoint
        float reachRadius = onLastWaypoint ? agent.stoppingDistance : agent.waypointRadius;
        
        if (distToWaypoint <= reachRadius) {
            if (onLastWaypoint) {
                // Reached final destination!
                agent.state = AgentState::Arrived;
                agent.currentSpeed = 0.0f;
                SE_LOG_DEBUG("[PathfindingAgent] Arrived at destination");
                if (agent.onArrived) agent.onArrived();
                return;
            }
            
            // Reached intermediate waypoint
            if (agent.onWaypointReached) {
                agent.onWaypointReached(targetWaypoint);
            }
            agent.currentWaypoint++;
        }


        // Update target waypoint after potential increment
        if (agent.currentWaypoint < static_cast<int32_t>(agent.currentPath.size())) {
            targetWaypoint = agent.currentPath[agent.currentWaypoint];
        } else {
            targetWaypoint = agent.targetPosition;
        }

        // Calculate direction to waypoint
        Vector3 direction = targetWaypoint - currentPos;
        direction.y = 0.0f;  // Keep movement horizontal
        
        float distance = glm::length(direction);
        if (distance > 0.001f) {
            direction = glm::normalize(direction);

            // Smooth acceleration/deceleration
            float targetSpeed = agent.moveSpeed;
            if (distance < agent.stoppingDistance * 2.0f) {
                targetSpeed *= (distance / (agent.stoppingDistance * 2.0f));
            }

            if (agent.currentSpeed < targetSpeed) {
                agent.currentSpeed = std::min(agent.currentSpeed + agent.acceleration * dt, targetSpeed);
            } else {
                agent.currentSpeed = std::max(agent.currentSpeed - agent.acceleration * dt, targetSpeed);
            }
        }
    }
}

}  // namespace nav
}  // namespace se
