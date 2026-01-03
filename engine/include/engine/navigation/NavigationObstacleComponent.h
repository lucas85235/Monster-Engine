#pragma once

#include "engine/navigation/PathNode.h"

#include <glm.hpp>

namespace se {
namespace nav {

using Vector3 = glm::vec3;

enum class ObstacleShape {
    Box,
    Cylinder
};

struct NavigationObstacleComponent {
    Vector3       size{1.0f, 1.0f, 1.0f};  // Half-extents for box, (radius, height, radius) for cylinder
    ObstacleShape shape   = ObstacleShape::Box;
    bool          carve   = true;           // Actually block navigation
    bool          enabled = true;

    // Runtime state (managed by NavigationSystem)
    bool          registered = false;
    Vector3       lastPosition{0.0f};
    GridCoord     gridMin;
    GridCoord     gridMax;
};

}  // namespace nav
}  // namespace se
