#include "engine/input/Input.h"

#include "engine/input/InputManager.h"

namespace se {

bool Input::IsKeyPressed(KeyCode key) {
    return InputManager::Get().IsKeyJustPressed(key);
}

bool Input::IsKeyDown(KeyCode key) {
    return InputManager::Get().IsKeyDown(key);
}

bool Input::IsKeyHeld(KeyCode key) {
    return InputManager::Get().IsKeyDown(key);
}

bool Input::IsKeyReleased(KeyCode key) {
    return InputManager::Get().IsKeyJustReleased(key);
}

void Input::UpdateKeyState(KeyCode /*key*/, KeyState /*newState*/) {
    // Deprecated legacy API: state is fully event-driven in InputManager.
}

Vector2 Input::GetMousePosition() {
    return InputManager::Get().GetMousePosition();
}

float Input::GetMouseX() {
    return GetMousePosition().x;
}

float Input::GetMouseY() {
    return GetMousePosition().y;
}

void Input::SetWindow(WindowHandle /*window*/) {
    // Deprecated legacy API: window ownership is handled by Application/Window.
}

}  // namespace se
