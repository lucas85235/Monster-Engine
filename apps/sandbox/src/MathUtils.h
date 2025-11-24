#pragma once

#include <glm.hpp>
#include <cmath>

namespace Math {

    /// Custom atan2 implementation for direction-to-angle conversion
    inline float Atan2(float y, float x) {
        if (x == 0.0f) {
            if (y > 0.0f) return 1.5707963f; // PI/2
            if (y < 0.0f) return -1.5707963f; // -PI/2
            return 0.0f;
        }
        
        float atan = std::atan(y / x);
        
        if (x < 0.0f) {
            if (y >= 0.0f)
                atan += 3.14159265f; // PI
            else
                atan -= 3.14159265f; // -PI
        }
        
        return atan;
    }

    /// Normalizes an angle to the [-180, 180] degree range
    inline float NormalizeAngle(float angle) {
        while (angle > 180.0f) angle -= 360.0f;
        while (angle < -180.0f) angle += 360.0f;
        return angle;
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
        float angle = Atan2(-x, -z);
        return angle * 57.2957795f; // Radians to degrees
    }

}
