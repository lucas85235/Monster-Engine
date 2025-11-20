#pragma once

#include <glm.hpp>
#include <unordered_map>

#include "engine/input/KeyCodes.h"

struct GLFWwindow;

namespace se {

struct KeyData {
    KeyCode  Key{};
    KeyState State    = KeyState::None;
    KeyState OldState = KeyState::None;
};

struct MouseButtonData {
    MouseButton Button{};
    KeyState    State    = KeyState::None;
    KeyState    OldState = KeyState::None;
};

class EventBus;

class Input {
   public:
    Input() = delete;

    static bool IsKeyPressed(KeyCode key);
    static bool IsKeyDown(KeyCode keycode);

    static bool IsKeyHeld(KeyCode key);

    static bool IsKeyReleased(KeyCode key);

    static void UpdateKeyState(KeyCode key, KeyState newState);

    static glm::vec2 GetMousePosition();

    static float GetMouseX();

    static float GetMouseY();

    // Internal: Used by Window to set the context
    static void SetWindow(GLFWwindow* window);

   private:
    static GLFWwindow* window_;

    inline static std::unordered_map<KeyCode, KeyData> key_states_;
};
}  // namespace se
