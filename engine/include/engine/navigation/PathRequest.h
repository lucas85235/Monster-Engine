#pragma once

#include "engine/navigation/PathNode.h"
#include "engine/navigation/AStar.h"
#include "engine/ecs/Entity.h"

#include <glm.hpp>
#include <cstdint>
#include <functional>
#include <vector>

namespace se {
namespace nav {

using Vector3 = glm::vec3;

enum class PathRequestStatus {
    Pending,    // Waiting in queue
    Computing,  // Being processed
    Complete,   // Path found
    Failed,     // No path exists
    Cancelled   // Request was cancelled
};

struct PathRequest {
    uint32_t            requestId = 0;
    Entity              requester;
    Vector3             start{0.0f};
    Vector3             goal{0.0f};
    PathfindingSettings settings;
    PathRequestStatus   status = PathRequestStatus::Pending;
    
    // Callback when path is ready (optional)
    std::function<void(const PathResult&)> onComplete;
};

}  // namespace nav
}  // namespace se
