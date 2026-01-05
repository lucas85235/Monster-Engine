#pragma once

// =============================================================================
// Monster Math Types
// Core math type definitions using GLM as the underlying implementation.
// =============================================================================

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>
#include <gtc/type_ptr.hpp>
#include <gtc/quaternion.hpp>
#include <gtx/quaternion.hpp>
#include <gtx/transform.hpp>
#include <gtx/matrix_decompose.hpp>
#include <gtx/euler_angles.hpp>

namespace luma {

// Vector types
using Vector2 = glm::vec2;
using Vector3 = glm::vec3;
using Vector4 = glm::vec4;

// Integer vector types
using IVector2 = glm::ivec2;
using IVector3 = glm::ivec3;
using IVector4 = glm::ivec4;

// Unsigned integer vector types
using UVector2 = glm::uvec2;
using UVector3 = glm::uvec3;
using UVector4 = glm::uvec4;

// Matrix types
using Matrix2 = glm::mat2;
using Matrix3 = glm::mat3;
using Matrix4 = glm::mat4;

// Quaternion
using Quaternion = glm::quat;

// Color (RGBA float)
using Color = glm::vec4;

// =============================================================================
// Constants
// =============================================================================

constexpr float PI        = 3.14159265358979323846f;
constexpr float TWO_PI    = PI * 2.0f;
constexpr float HALF_PI   = PI * 0.5f;
constexpr float DEG2RAD   = PI / 180.0f;
constexpr float RAD2DEG   = 180.0f / PI;
constexpr float EPSILON   = 1e-6f;

// =============================================================================
// Identity constants
// =============================================================================

inline const Matrix4 IDENTITY_MATRIX4 = Matrix4(1.0f);
inline const Matrix3 IDENTITY_MATRIX3 = Matrix3(1.0f);
inline const Quaternion IDENTITY_QUATERNION = Quaternion(1.0f, 0.0f, 0.0f, 0.0f);

// Direction vectors (right-handed, Y-up coordinate system)
inline const Vector3 VECTOR3_ZERO    = Vector3(0.0f, 0.0f, 0.0f);
inline const Vector3 VECTOR3_ONE     = Vector3(1.0f, 1.0f, 1.0f);
inline const Vector3 VECTOR3_UP      = Vector3(0.0f, 1.0f, 0.0f);
inline const Vector3 VECTOR3_DOWN    = Vector3(0.0f, -1.0f, 0.0f);
inline const Vector3 VECTOR3_RIGHT   = Vector3(1.0f, 0.0f, 0.0f);
inline const Vector3 VECTOR3_LEFT    = Vector3(-1.0f, 0.0f, 0.0f);
inline const Vector3 VECTOR3_FORWARD = Vector3(0.0f, 0.0f, -1.0f);
inline const Vector3 VECTOR3_BACK    = Vector3(0.0f, 0.0f, 1.0f);

} // namespace luma
