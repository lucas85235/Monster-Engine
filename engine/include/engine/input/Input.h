#pragma once

#include <glm.hpp>

#include "Engine.h"
#include "engine/input/KeyCodes.h"

namespace se {

// Compatibility wrapper kept for transitional callsites.
// New code should use InputManager directly.
class Input {
   public:
    Input() = delete;

    static bool IsKeyPressed(KeyCode key);
    static bool IsKeyDown(KeyCode key);
    static bool IsKeyHeld(KeyCode key);
    static bool IsKeyReleased(KeyCode key);

    static void UpdateKeyState(KeyCode key, KeyState newState);

    static Vector2 GetMousePosition();
    static float   GetMouseX();
    static float   GetMouseY();

    static void SetWindow(WindowHandle window);
};

}  // namespace se
