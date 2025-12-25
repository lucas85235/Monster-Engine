#pragma once
/**
 * MousePicker.h - Ray picking logic for entity selection.
 *
 * Performs screen-to-world ray casting and AABB intersection tests
 * to determine which entity the user clicked on.
 */

#include <limits>

#include "Engine.h"
#include "engine/Camera.h"
#include "engine/ecs/Entity.h"
#include "engine/ecs/Scene.h"

namespace mst {

class MousePicker {
public:
    se::Entity Pick(const Camera& camera, se::Scene& scene,
                    float mouseX, float mouseY, 
                    float viewportWidth, float viewportHeight);

    se::Vector3 ScreenToWorldRay(const Camera& camera,
                                  float mouseX, float mouseY,
                                  float viewportWidth, float viewportHeight);

private:
    bool RayIntersectsAABB(const se::Vector3& rayOrigin, const se::Vector3& rayDir,
                            const se::Vector3& boxMin, const se::Vector3& boxMax, float& t);
};

}  // namespace mst
