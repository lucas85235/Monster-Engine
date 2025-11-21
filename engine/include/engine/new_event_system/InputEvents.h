#pragma once

#include "engine/input/KeyCodes.h"

namespace se {

struct NewMouseMovedEvent {
    float x, y;
};

struct NewMouseButtonPressedEvent {
    MouseButton button;
};

struct NewMouseButtonReleasedEvent {
    MouseButton button;
};

struct NewMouseScrolledEvent {
    float xOffset, yOffset;
};

struct NewKeyPressedEvent {
    KeyCode key;
    int scancode;
    int mods;
};

struct NewKeyReleasedEvent {
    KeyCode key;
    int scancode;
    int mods;
};

struct NewCharTypedEvent {
    unsigned int character;
};

}  // namespace se
