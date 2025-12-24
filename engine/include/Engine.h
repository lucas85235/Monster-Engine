#pragma once

#include "se_pch.h"

struct GLFWwindow;
struct GLFWmonitor;
struct GLFWcursor;

namespace se {

// Math Types - must be defined before other includes that use them
using Vector2    = glm::vec2;
using Vector3    = glm::vec3;
using Vector4    = glm::vec4;
using Matrix4    = glm::mat4;
using Matrix3    = glm::mat3;
using Matrix2    = glm::mat2;
using Quaternion = glm::quat;
using Color      = glm::vec4;

// Input Types
using MouseButton = uint16_t;
using KeyCode     = uint16_t;

// Windowing Types
using WindowHandle  = GLFWwindow*;
using MonitorHandle = GLFWmonitor*;
using CursorHandle  = GLFWcursor*;

// Event system types
using EventTypeId = std::size_t;

namespace detail {
inline EventTypeId GenerateTypeId() {
    static EventTypeId counter = 0;
    return counter++;
}

template <typename T>
EventTypeId GetTypeId() {
    static EventTypeId id = GenerateTypeId();
    return id;
}
}  // namespace detail

// Smart pointer aliases
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

// Utility macros
#define BIT(x) (1 << x)
#define SE_BIND_EVENT_FN(fn)                                    \
    [this](auto&&... args) -> decltype(auto) {                  \
        return this->fn(std::forward<decltype(args)>(args)...); \
    }

#define DEBUG

}  // namespace se

// ECS includes - must come after se namespace types are defined
#include "engine/ecs/Component.h"
#include "engine/ecs/Entity.h"
#include "engine/ecs/Scene.h"
