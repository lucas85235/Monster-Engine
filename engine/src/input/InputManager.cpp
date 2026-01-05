#include "engine/input/InputManager.h"

#include <GLFW/glfw3.h>

#include <algorithm>

#include "engine/Application.h"
#include "engine/Log.h"
#include "engine/input/GamepadManager.h"

namespace se {

void InputManager::Init(EventBus* eventBus) {
    eventBus_ = eventBus;

    GamepadManager::Get().Init(eventBus);

    SE_LOG_INFO("InputManager initialized");
}

void InputManager::Shutdown() {
    GamepadManager::Get().Shutdown();
    SE_LOG_INFO("InputManager shutdown");
}

void InputManager::Update() {
    // Reset keyboard/mouse "Just" states
    for (auto& [key, state] : keyStates_) {
        state.JustPressed  = false;
        state.JustReleased = false;
    }
    for (auto& [btn, state] : mouseButtonStates_) {
        state.JustPressed  = false;
        state.JustReleased = false;
    }

    mouseDelta_  = {0.0f, 0.0f};
    scrollDelta_ = 0.0f;

    // Update gamepad state
    GamepadManager::Get().Update();
}

void InputManager::SetCursorMode(CursorMode mode) {
    auto&        app    = Application::Get();
    WindowHandle window = app.GetWindow().GetNativeWindow();

    int glfwMode = GLFW_CURSOR_NORMAL;
    switch (mode) {
        case CursorMode::Normal:
            glfwMode = GLFW_CURSOR_NORMAL;
            break;
        case CursorMode::Hidden:
            glfwMode = GLFW_CURSOR_HIDDEN;
            break;
        case CursorMode::Locked:
            glfwMode = GLFW_CURSOR_DISABLED;
            break;
    }

    glfwSetInputMode(window, GLFW_CURSOR, glfwMode);
}

// Keyboard/Mouse Bindings
void InputManager::BindAction(const std::string& name, KeyCode key) {
    actionBindings_.push_back({name, key});
}

void InputManager::BindAxis(const std::string& name, KeyCode key, float scale) {
    axisBindings_.push_back({name, key, scale});
}

void InputManager::UnbindAction(const std::string& name) {
    actionBindings_.erase(
        std::remove_if(actionBindings_.begin(), actionBindings_.end(),
                       [&](const ActionBinding& binding) { return binding.Name == name; }),
        actionBindings_.end());
}

void InputManager::UnbindAxis(const std::string& name) {
    axisBindings_.erase(
        std::remove_if(axisBindings_.begin(), axisBindings_.end(),
                       [&](const AxisBinding& binding) { return binding.Name == name; }),
        axisBindings_.end());
}

// Gamepad Bindings
void InputManager::BindGamepadAction(const std::string& name, GamepadButton button) {
    gamepadActionBindings_.push_back({name, button});
}

void InputManager::BindGamepadAxis(const std::string& name, GamepadAxis axis, float scale, bool invert) {
    gamepadAxisBindings_.push_back({name, axis, scale, invert});
}

void InputManager::UnbindGamepadAction(const std::string& name) {
    gamepadActionBindings_.erase(
        std::remove_if(gamepadActionBindings_.begin(), gamepadActionBindings_.end(),
                       [&](const GamepadActionBinding& binding) { return binding.Name == name; }),
        gamepadActionBindings_.end());
}

void InputManager::UnbindGamepadAxis(const std::string& name) {
    gamepadAxisBindings_.erase(
        std::remove_if(gamepadAxisBindings_.begin(), gamepadAxisBindings_.end(),
                       [&](const GamepadAxisBinding& binding) { return binding.Name == name; }),
        gamepadAxisBindings_.end());
}

// Unified Queries
bool InputManager::IsActionPressed(const std::string& name) const {
    // Check keyboard bindings
    for (const auto& binding : actionBindings_) {
        if (binding.Name == name) {
            if (IsKeyDown(binding.Key)) return true;
        }
    }
    // Check gamepad bindings
    for (const auto& binding : gamepadActionBindings_) {
        if (binding.Name == name) {
            if (IsGamepadButtonDown(binding.Button)) return true;
        }
    }
    return false;
}

bool InputManager::IsActionJustPressed(const std::string& name) const {
    // Check keyboard bindings
    for (const auto& binding : actionBindings_) {
        if (binding.Name == name) {
            auto it = keyStates_.find(binding.Key);
            if (it != keyStates_.end() && it->second.JustPressed) return true;
        }
    }
    // Check gamepad bindings
    for (const auto& binding : gamepadActionBindings_) {
        if (binding.Name == name) {
            if (IsGamepadButtonPressed(binding.Button)) return true;
        }
    }
    return false;
}

bool InputManager::IsActionJustReleased(const std::string& name) const {
    // Check keyboard bindings
    for (const auto& binding : actionBindings_) {
        if (binding.Name == name) {
            auto it = keyStates_.find(binding.Key);
            if (it != keyStates_.end() && it->second.JustReleased) return true;
        }
    }
    // Check gamepad bindings
    for (const auto& binding : gamepadActionBindings_) {
        if (binding.Name == name) {
            if (IsGamepadButtonReleased(binding.Button)) return true;
        }
    }
    return false;
}

float InputManager::GetAxis(const std::string& name) const {
    float value = 0.0f;

    // Keyboard/Mouse axis bindings
    for (const auto& binding : axisBindings_) {
        if (binding.Name == name) {
            if (binding.Key == Key::MouseX) {
                value += mouseDelta_.x * binding.Scale;
            } else if (binding.Key == Key::MouseY) {
                value += mouseDelta_.y * binding.Scale;
            } else if (binding.Key == Key::MouseScrollY) {
                value += scrollDelta_ * binding.Scale;
            } else if (IsKeyDown(binding.Key)) {
                value += binding.Scale;
            }
        }
    }

    // Gamepad axis bindings
    for (const auto& binding : gamepadAxisBindings_) {
        if (binding.Name == name) {
            float axisValue = GetGamepadAxis(binding.Axis);
            if (binding.Invert) axisValue = -axisValue;
            value += axisValue * binding.Scale;
        }
    }

    return value;
}

// Raw Keyboard/Mouse Input
bool InputManager::IsKeyDown(KeyCode key) const {
    auto it = keyStates_.find(key);
    return it != keyStates_.end() && it->second.IsDown;
}

bool InputManager::IsMouseButtonDown(MouseButton button) const {
    auto it = mouseButtonStates_.find(button);
    return it != mouseButtonStates_.end() && it->second.IsDown;
}

Vector2 InputManager::GetMousePosition() const {
    return mousePosition_;
}

Vector2 InputManager::GetMouseDelta() const {
    return mouseDelta_;
}

// Raw Gamepad Input
bool InputManager::IsGamepadButtonDown(GamepadButton button) const {
    return GamepadManager::Get().IsButtonDown(button);
}

bool InputManager::IsGamepadButtonPressed(GamepadButton button) const {
    return GamepadManager::Get().IsButtonPressed(button);
}

bool InputManager::IsGamepadButtonReleased(GamepadButton button) const {
    return GamepadManager::Get().IsButtonReleased(button);
}

float InputManager::GetGamepadAxis(GamepadAxis axis) const {
    return GamepadManager::Get().GetAxis(axis);
}

bool InputManager::IsGamepadConnected(GamepadId id) const {
    return GamepadManager::Get().IsConnected(id);
}

// Event Handlers
void InputManager::OnKeyPressed(KeyCode key) {
    auto& state = keyStates_[key];
    if (!state.IsDown) {
        state.IsDown      = true;
        state.JustPressed = true;
    }
}

void InputManager::OnKeyReleased(KeyCode key) {
    auto& state = keyStates_[key];
    if (state.IsDown) {
        state.IsDown       = false;
        state.JustReleased = true;
    }
}

void InputManager::OnMouseButtonPressed(MouseButton button) {
    auto& state = mouseButtonStates_[button];
    if (!state.IsDown) {
        state.IsDown      = true;
        state.JustPressed = true;
    }
}

void InputManager::OnMouseButtonReleased(MouseButton button) {
    auto& state = mouseButtonStates_[button];
    if (state.IsDown) {
        state.IsDown       = false;
        state.JustReleased = true;
    }
}

void InputManager::OnMouseMoved(float x, float y) {
    if (firstMouse_) {
        lastMousePosition_ = {x, y};
        firstMouse_        = false;
    }

    mousePosition_ = {x, y};
    mouseDelta_ += mousePosition_ - lastMousePosition_;
    lastMousePosition_ = mousePosition_;
}

void InputManager::OnMouseScrolled(float yOffset) {
    scrollDelta_ += yOffset;
}

}  // namespace se
