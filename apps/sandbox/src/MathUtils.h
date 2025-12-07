#pragma once

#include <glm.hpp>
#include <cmath>

namespace Math {

    /// Normalizes an angle to the [-180, 180] degree range
    inline float NormalizeAngle(float angle) {
        angle = std::fmod(angle + 180.0f, 360.0f);
        if (angle < 0.0f) angle += 360.0f;
        return angle - 180.0f;
    }

    /// Interpolates between two yaw angles taking the shortest rotational path
    /// @param currentYaw Current angle in degrees
    /// @param targetYaw Target angle in degrees
    /// @param t Interpolation factor [0, 1]
    /// @return Interpolated angle in degrees
    inline float InterpolateYaw(float currentYaw, float targetYaw, float t) {
        t = glm::clamp(t, 0.0f, 1.0f);
        
        currentYaw = NormalizeAngle(currentYaw);
        targetYaw = NormalizeAngle(targetYaw);
        
        float diff = targetYaw - currentYaw;
        
        // Take shortest path
        if (diff > 180.0f) diff -= 360.0f;
        if (diff < -180.0f) diff += 360.0f;
        
        return currentYaw + diff * t;
    }

    /// Calculates yaw angle from a direction vector (assumes forward is -Z)
    /// @param x X component of direction
    /// @param z Z component of direction
    /// @return Yaw angle in degrees
    inline float CalculateYawFromDirection(float x, float z) {
        // Use std::atan2 for robust angle calculation
        // We use (-x, -z) to match the coordinate system where -Z is 0 degrees (Forward)
        // and +X is -90 degrees (Right).
        float angle = std::atan2(-x, -z);
        return glm::degrees(angle);
    }
}
