#pragma once

// =============================================================================
// Monster Math Utilities
// Common math operations and helper functions.
// =============================================================================

#include "Types.h"
#include <cmath>
#include <algorithm>

namespace luma {

// =============================================================================
// Angle Utilities
// =============================================================================

/// Normalizes an angle to the [-180, 180] degree range
[[nodiscard]] inline float NormalizeAngleDeg(float angle) {
    angle = std::fmod(angle + 180.0f, 360.0f);
    if (angle < 0.0f) angle += 360.0f;
    return angle - 180.0f;
}

/// Normalizes an angle to the [-PI, PI] radian range
[[nodiscard]] inline float NormalizeAngleRad(float angle) {
    angle = std::fmod(angle + PI, TWO_PI);
    if (angle < 0.0f) angle += TWO_PI;
    return angle - PI;
}

/// Interpolates between two yaw angles taking the shortest rotational path
/// @param current Current angle in degrees
/// @param target Target angle in degrees
/// @param t Interpolation factor [0, 1]
/// @return Interpolated angle in degrees
[[nodiscard]] inline float LerpAngleDeg(float current, float target, float t) {
    t = glm::clamp(t, 0.0f, 1.0f);
    
    float diff = NormalizeAngleDeg(target - current);
    return current + diff * t;
}

/// Interpolates between two angles in radians taking the shortest path
[[nodiscard]] inline float LerpAngleRad(float current, float target, float t) {
    t = glm::clamp(t, 0.0f, 1.0f);
    
    float diff = NormalizeAngleRad(target - current);
    return current + diff * t;
}

/// Calculates yaw angle from a direction vector (assumes forward is -Z)
/// @param x X component of direction
/// @param z Z component of direction
/// @return Yaw angle in degrees
[[nodiscard]] inline float YawFromDirection(float x, float z) {
    return glm::degrees(std::atan2(x, z));
}

/// Calculates yaw angle from a direction vector
[[nodiscard]] inline float YawFromDirection(const Vector3& dir) {
    return YawFromDirection(dir.x, dir.z);
}

// =============================================================================
// Interpolation
// =============================================================================

/// Linear interpolation
template<typename T>
[[nodiscard]] inline T Lerp(const T& a, const T& b, float t) {
    return glm::mix(a, b, t);
}

/// Smoothstep interpolation
[[nodiscard]] inline float Smoothstep(float edge0, float edge1, float x) {
    x = glm::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

/// Smoother step (Ken Perlin's improved smoothstep)
[[nodiscard]] inline float Smootherstep(float edge0, float edge1, float x) {
    x = glm::clamp((x - edge0) / (edge1 - edge0), 0.0f, 1.0f);
    return x * x * x * (x * (x * 6.0f - 15.0f) + 10.0f);
}

/// Exponential decay interpolation (frame-rate independent)
/// @param current Current value
/// @param target Target value
/// @param decay Decay rate (higher = faster)
/// @param dt Delta time
template<typename T>
[[nodiscard]] inline T ExpDecay(const T& current, const T& target, float decay, float dt) {
    return target + (current - target) * std::exp(-decay * dt);
}

// =============================================================================
// Clamping and Remapping
// =============================================================================

/// Clamp value between min and max
template<typename T>
[[nodiscard]] inline T Clamp(const T& value, const T& minVal, const T& maxVal) {
    return glm::clamp(value, minVal, maxVal);
}

/// Clamp value between 0 and 1
[[nodiscard]] inline float Saturate(float value) {
    return glm::clamp(value, 0.0f, 1.0f);
}

/// Remap value from one range to another
[[nodiscard]] inline float Remap(float value, float inMin, float inMax, float outMin, float outMax) {
    float t = (value - inMin) / (inMax - inMin);
    return outMin + t * (outMax - outMin);
}

/// Remap value from one range to another with clamping
[[nodiscard]] inline float RemapClamped(float value, float inMin, float inMax, float outMin, float outMax) {
    float t = Saturate((value - inMin) / (inMax - inMin));
    return outMin + t * (outMax - outMin);
}

// =============================================================================
// Comparison
// =============================================================================

/// Check if two floats are approximately equal
[[nodiscard]] inline bool ApproxEqual(float a, float b, float epsilon = EPSILON) {
    return std::abs(a - b) < epsilon;
}

/// Check if a float is approximately zero
[[nodiscard]] inline bool ApproxZero(float value, float epsilon = EPSILON) {
    return std::abs(value) < epsilon;
}

/// Check if two vectors are approximately equal
[[nodiscard]] inline bool ApproxEqual(const Vector3& a, const Vector3& b, float epsilon = EPSILON) {
    return glm::length(a - b) < epsilon;
}

// =============================================================================
// Vector Operations
// =============================================================================

/// Project vector a onto vector b
[[nodiscard]] inline Vector3 Project(const Vector3& a, const Vector3& b) {
    return glm::dot(a, b) / glm::dot(b, b) * b;
}

/// Reject vector a from vector b (perpendicular component)
[[nodiscard]] inline Vector3 Reject(const Vector3& a, const Vector3& b) {
    return a - Project(a, b);
}

/// Safe normalize (returns zero vector if input is too small)
[[nodiscard]] inline Vector3 SafeNormalize(const Vector3& v, float epsilon = EPSILON) {
    float len = glm::length(v);
    if (len < epsilon) return VECTOR3_ZERO;
    return v / len;
}

/// Get the signed angle between two vectors around an axis (in degrees)
[[nodiscard]] inline float SignedAngle(const Vector3& from, const Vector3& to, const Vector3& axis) {
    float angle = glm::degrees(std::acos(glm::clamp(glm::dot(from, to), -1.0f, 1.0f)));
    float sign = glm::dot(axis, glm::cross(from, to));
    return sign < 0.0f ? -angle : angle;
}

// =============================================================================
// Matrix Utilities
// =============================================================================

/// Extract position from transformation matrix
[[nodiscard]] inline Vector3 GetTranslation(const Matrix4& m) {
    return Vector3(m[3]);
}

/// Extract scale from transformation matrix
[[nodiscard]] inline Vector3 GetScale(const Matrix4& m) {
    return Vector3(
        glm::length(Vector3(m[0])),
        glm::length(Vector3(m[1])),
        glm::length(Vector3(m[2]))
    );
}

/// Extract rotation quaternion from transformation matrix
[[nodiscard]] inline Quaternion GetRotation(const Matrix4& m) {
    Vector3 scale = GetScale(m);
    Matrix3 rotMat(
        Vector3(m[0]) / scale.x,
        Vector3(m[1]) / scale.y,
        Vector3(m[2]) / scale.z
    );
    return glm::quat_cast(rotMat);
}

/// Decompose transformation matrix into components
inline void Decompose(const Matrix4& m, Vector3& outPosition, Quaternion& outRotation, Vector3& outScale) {
    outPosition = GetTranslation(m);
    outScale = GetScale(m);
    outRotation = GetRotation(m);
}

/// Compose transformation matrix from components
[[nodiscard]] inline Matrix4 Compose(const Vector3& position, const Quaternion& rotation, const Vector3& scale) {
    Matrix4 result = glm::translate(IDENTITY_MATRIX4, position);
    result *= glm::toMat4(rotation);
    result = glm::scale(result, scale);
    return result;
}

} // namespace luma
