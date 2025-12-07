#pragma once
#include "Engine.h"
#include "engine/input/KeyCodes.h"

namespace se {

struct KeyEvent {
    KeyCode key;
    KeyEvent(KeyCode k) : key(k) {}
};

struct KeyPressedEvent : public KeyEvent {
    bool isRepeat;
    KeyPressedEvent(KeyCode k, bool repeat = false) : KeyEvent(k), isRepeat(repeat) {}
};

struct KeyReleasedEvent : public KeyEvent {
    KeyReleasedEvent(KeyCode k) : KeyEvent(k) {}
};

// Base para eventos de Mouse
struct MouseMovedEvent {
    float x, y;
    MouseMovedEvent(float x, float y) : x(x), y(y) {}
};

struct MouseButtonPressedEvent {
    MouseButton button;
    MouseButtonPressedEvent(MouseButton btn) : button(btn) {}
};

struct MouseButtonReleasedEvent {
    MouseButton button;
    MouseButtonReleasedEvent(MouseButton btn) : button(btn) {}
};

struct MouseScrolledEvent {
    float xOffset, yOffset;
    MouseScrolledEvent(float x, float y) : xOffset(x), yOffset(y) {}
};

}