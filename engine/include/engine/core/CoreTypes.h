#pragma once

/**
 * CoreTypes.h - Central type definitions for SimpleEngine
 * 
 * This header contains all fundamental type aliases used throughout the engine.
 * It MUST NOT include any other engine headers to avoid circular dependencies.
 * All other engine headers that need these types should include this file.
 */

#include <cstdint>
#include <memory>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm.hpp>
#include <gtc/quaternion.hpp>

struct GLFWwindow;
struct GLFWmonitor;
struct GLFWcursor;

namespace se {

// =============================================================================
// Math Types
// =============================================================================
using Vector2    = glm::vec2;
using Vector3    = glm::vec3;
using Vector4    = glm::vec4;
using Matrix4    = glm::mat4;
using Matrix3    = glm::mat3;
using Matrix2    = glm::mat2;
using Quaternion = glm::quat;
using Color      = glm::vec4;

// =============================================================================
// Input Types
// =============================================================================
using MouseButton = uint16_t;
using KeyCode     = uint16_t;

// =============================================================================
// Windowing Types
// =============================================================================
using WindowHandle  = GLFWwindow*;
using MonitorHandle = GLFWmonitor*;
using CursorHandle  = GLFWcursor*;

// =============================================================================
// Event System Types  
// =============================================================================
using EventTypeId = std::size_t;

// =============================================================================
// Smart Pointer Aliases
// =============================================================================
template <typename T>
using Scope = std::unique_ptr<T>;

template <typename T, typename... Args>
constexpr Scope<T> CreateScope(Args&&... args) {
    return std::make_unique<T>(std::forward<Args>(args)...);
}

template <typename T>
using Ref = std::shared_ptr<T>;

template <typename T, typename... Args>
constexpr Ref<T> CreateRef(Args&&... args) {
    return std::make_shared<T>(std::forward<Args>(args)...);
}

// =============================================================================
// Utility Macros
// =============================================================================
#define BIT(x) (1 << x)
#define SE_BIND_EVENT_FN(fn)                                    \
    [this](auto&&... args) -> decltype(auto) {                  \
        return this->fn(std::forward<decltype(args)>(args)...); \
    }

#define DEBUG

}  // namespace se
