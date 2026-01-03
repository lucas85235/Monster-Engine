#pragma once

// Navigation System - Main include file
// Include this single header to access all navigation functionality

#include "engine/navigation/PathNode.h"
#include "engine/navigation/NavigationGrid.h"
#include "engine/navigation/AStar.h"
#include "engine/navigation/PathRequest.h"
#include "engine/navigation/NavigationSystem.h"
#include "engine/navigation/NavigationDebug.h"
#include "engine/navigation/PathfindingAgentComponent.h"
#include "engine/navigation/NavigationObstacleComponent.h"

namespace se {

// Convenience alias for the navigation namespace
namespace nav {

// Quick setup helper
inline NavigationGridSettings CreateDefaultGridSettings(
    float worldSize = 100.0f,
    float cellSize = 1.0f,
    glm::vec3 center = {0.0f, 0.0f, 0.0f}) {
    
    NavigationGridSettings settings;
    float halfSize = worldSize * 0.5f;
    settings.worldOrigin = center - glm::vec3(halfSize, 0.0f, halfSize);
    settings.width = static_cast<int32_t>(worldSize / cellSize);
    settings.height = static_cast<int32_t>(worldSize / cellSize);
    settings.cellSize = cellSize;
    return settings;
}

}  // namespace nav
}  // namespace se
