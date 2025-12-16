#pragma once

#include <cstdint>

namespace se {

// Type aliases for input codes
using KeyCode = uint16_t;
using MouseButton = uint16_t;

// ==================== Window Events ====================

struct WindowResizeEvent {
    uint32_t width;
    uint32_t height;
};

struct WindowCloseEvent {};

struct WindowMinimizeEvent {
    bool minimized;
};

struct WindowFocusEvent {
    bool focused;
};

struct WindowMovedEvent {
    int x;
    int y;
};

// ==================== Keyboard Events ====================

struct KeyPressedEvent {
    KeyCode keyCode;
    int repeatCount;
    
    bool IsRepeat() const { return repeatCount > 0; }
};

struct KeyReleasedEvent {
    KeyCode keyCode;
};

struct KeyTypedEvent {
    KeyCode keyCode;
};

// ==================== Mouse Events ====================

struct MouseMovedEvent {
    float x;
    float y;
};

struct MouseScrolledEvent {
    float xOffset;
    float yOffset;
};

struct MouseButtonPressedEvent {
    MouseButton button;
};

struct MouseButtonReleasedEvent {
    MouseButton button;
};

// ==================== Application Events ====================

struct AppTickEvent {};
struct AppUpdateEvent {};
struct AppRenderEvent {};

}  // namespace se
