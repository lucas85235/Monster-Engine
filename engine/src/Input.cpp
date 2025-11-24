#include "engine/Input.h"
#include <GLFW/glfw3.h>

namespace se {

GLFWwindow* Input::window_ = nullptr;

void Input::SetWindow(GLFWwindow* window) {
    window_ = window;
}

bool Input::IsKeyPressed(int keycode) {
    if (!window_)
        return false;
    int state = glfwGetKey(window_, keycode);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool Input::IsMouseButtonPressed(int button) {
    if (!window_)
        return false;
    int state = glfwGetMouseButton(window_, button);
    return state == GLFW_PRESS;
}

glm::vec2 Input::GetMousePosition() {
    if (!window_)
        return {0.0f, 0.0f};
    double xpos, ypos;
    glfwGetCursorPos(window_, &xpos, &ypos);
    SE_LOG_DEBUG("Mouse position: ({}, {})", xpos, ypos);
    return {static_cast<float>(xpos), static_cast<float>(ypos)};
}

float Input::GetMouseX() {
    return GetMousePosition().x;
}

float Input::GetMouseY() {
    return GetMousePosition().y;
}

} // namespace se