#pragma once

#include <cstdint>

namespace se {

// Type aliases for input codes
using KeyCode     = uint16_t;
using MouseButton = uint16_t;

// ==================== Window Events ====================

struct WindowResizeEvent {
    uint32_t width;
    uint32_t height;
    WindowResizeEvent(uint32_t w, uint32_t h) : width(w), height(h) {}
};

struct WindowCloseEvent {};

struct WindowMinimizeEvent {
    bool minimized;
    WindowMinimizeEvent(bool m) : minimized(m) {}
};

struct WindowFocusEvent {
    bool focused;
    WindowFocusEvent(bool f) : focused(f) {}
};

struct WindowMovedEvent {
    int x;
    int y;
    WindowMovedEvent(int x_, int y_) : x(x_), y(y_) {}
};

// ==================== Keyboard Events ====================

struct KeyPressedEvent {
    KeyCode keyCode;
    int     repeatCount;

    KeyPressedEvent(KeyCode k, int r) : keyCode(k), repeatCount(r) {}
    bool IsRepeat() const {
        return repeatCount > 0;
    }
};

struct KeyReleasedEvent {
    KeyCode keyCode;
    KeyReleasedEvent(KeyCode k) : keyCode(k) {}
};

struct KeyTypedEvent {
    KeyCode keyCode;
    KeyTypedEvent(KeyCode k) : keyCode(k) {}
};

// ==================== Mouse Events ====================

struct MouseMovedEvent {
    float x;
    float y;
    MouseMovedEvent(float x_, float y_) : x(x_), y(y_) {}
};

struct MouseScrolledEvent {
    float xOffset;
    float yOffset;
    MouseScrolledEvent(float xOff, float yOff) : xOffset(xOff), yOffset(yOff) {}
};

struct MouseButtonPressedEvent {
    MouseButton button;
    MouseButtonPressedEvent(MouseButton b) : button(b) {}
};

struct MouseButtonReleasedEvent {
    MouseButton button;
    MouseButtonReleasedEvent(MouseButton b) : button(b) {}
};

// ==================== Application Events ====================

struct AppTickEvent {};
struct AppUpdateEvent {};
struct AppRenderEvent {};

}  // namespace se
