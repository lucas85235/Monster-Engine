#include "engine/input/Input.h"

#include <GLFW/glfw3.h>

#include "engine/Application.h"

namespace se {

GLFWwindow* Input::window_ = nullptr;

void Input::SetWindow(GLFWwindow* window) {
    window_ = window;
}

bool Input::IsKeyPressed(KeyCode key) {
    return key_states_.contains(key) && key_states_[key].State == KeyState::Pressed;
}

bool Input::IsKeyDown(KeyCode keycode) {
    auto& window = static_cast<Window&>(Application::Get().GetWindow());
    auto  state  = glfwGetKey(window.GetNativeWindow(), keycode);
    return state == GLFW_PRESS || state == GLFW_REPEAT;
}

bool Input::IsKeyHeld(KeyCode key) {
    return key_states_.contains(key) && key_states_[key].State == KeyState::Held;
}

bool Input::IsKeyReleased(KeyCode key) {
    return key_states_.contains(key) && key_states_[key].State == KeyState::Released;
}

void Input::UpdateKeyState(KeyCode key, KeyState newState) {
    auto& keyData    = key_states_[key];
    keyData.Key      = key;
    keyData.OldState = keyData.State;
    keyData.State    = newState;
}

glm::vec2 Input::GetMousePosition() {
    if (!window_) return {0.0f, 0.0f};
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

}  // namespace se