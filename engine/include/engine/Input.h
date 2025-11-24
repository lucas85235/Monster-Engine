#pragma once

#include <glm.hpp>

struct GLFWwindow;

namespace se {
class Input {
  public:
    static bool IsKeyPressed(int keycode);

    static bool IsKeyDown(int keycode);

    bool IsKeyReleased(int keycode);

    static bool IsMouseButtonPressed(int button);

    static glm::vec2 GetMousePosition();

    static float GetMouseX();

    static float GetMouseY();

    // Internal: Used by Window to set the context
    static void SetWindow(GLFWwindow* window);

  private:
    Input() = delete;

    static GLFWwindow* window_;

    static std::unordered_map<int, bool> current_key_states_;
    static std::unordered_map<int, bool> last_key_states_;
};
} // namespace se
