#include "engine/input/InputManager.h"
#include "engine/Log.h"
#include "engine/Application.h"
#include <algorithm>
#include <GLFW/glfw3.h> // For raw input queries if needed, but we try to rely on events

namespace se {

void InputManager::Init() {
    // Initialize default bindings or state if needed
    SE_LOG_INFO("InputManager Initialized");
}

void InputManager::Update() {
    // Reset "Just" states
    for (auto& [key, state] : keyStates_) {
        state.JustPressed = false;
        state.JustReleased = false;
    }
    for (auto& [btn, state] : mouseButtonStates_) {
        state.JustPressed = false;
        state.JustReleased = false;
    }

    mouseDelta_ = {0.0f, 0.0f};
}

void InputManager::SetCursorMode(CursorMode mode) {
    auto& app = Application::Get();
    WindowHandle window = app.GetWindow().GetNativeWindow();
    
    int glfwMode = GLFW_CURSOR_NORMAL;
    switch (mode) {
        case CursorMode::Normal: glfwMode = GLFW_CURSOR_NORMAL; break;
        case CursorMode::Hidden: glfwMode = GLFW_CURSOR_HIDDEN; break;
        case CursorMode::Locked: glfwMode = GLFW_CURSOR_DISABLED; break;
    }
    
    glfwSetInputMode(window, GLFW_CURSOR, glfwMode);
}

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

bool InputManager::IsActionPressed(const std::string& name) const {
    for (const auto& binding : actionBindings_) {
        if (binding.Name == name) {
            if (IsKeyDown(binding.Key)) return true;
        }
    }
    return false;
}

bool InputManager::IsActionJustPressed(const std::string& name) const {
    for (const auto& binding : actionBindings_) {
        if (binding.Name == name) {
            auto it = keyStates_.find(binding.Key);
            if (it != keyStates_.end() && it->second.JustPressed) return true;
        }
    }
    return false;
}

bool InputManager::IsActionJustReleased(const std::string& name) const {
    for (const auto& binding : actionBindings_) {
        if (binding.Name == name) {
            auto it = keyStates_.find(binding.Key);
            if (it != keyStates_.end() && it->second.JustReleased) return true;
        }
    }
    return false;
}

float InputManager::GetAxis(const std::string& name) const {
    float value = 0.0f;
    for (const auto& binding : axisBindings_) {
        if (binding.Name == name) {
            if (binding.Key == Key::MouseX) {
                value += mouseDelta_.x * binding.Scale;
            } else if (binding.Key == Key::MouseY) {
                value += mouseDelta_.y * binding.Scale;
            } else if (IsKeyDown(binding.Key)) {
                value += binding.Scale;
            }
        }
    }
    return value;
}

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

void InputManager::OnKeyPressed(KeyCode key) {
    auto& state = keyStates_[key];
    if (!state.IsDown) {
        state.IsDown = true;
        state.JustPressed = true;
    }
}

void InputManager::OnKeyReleased(KeyCode key) {
    auto& state = keyStates_[key];
    if (state.IsDown) {
        state.IsDown = false;
        state.JustReleased = true;
    }
}

void InputManager::OnMouseButtonPressed(MouseButton button) {
    auto& state = mouseButtonStates_[button];
    if (!state.IsDown) {
        state.IsDown = true;
        state.JustPressed = true;
    }
}

void InputManager::OnMouseButtonReleased(MouseButton button) {
    auto& state = mouseButtonStates_[button];
    if (state.IsDown) {
        state.IsDown = false;
        state.JustReleased = true;
    }
}

void InputManager::OnMouseMoved(float x, float y) {
    if (firstMouse_) {
        lastMousePosition_ = {x, y};
        firstMouse_ = false;
    }

    mousePosition_ = {x, y};
    mouseDelta_ += mousePosition_ - lastMousePosition_;
    lastMousePosition_ = mousePosition_;
    
    // Y inverted in many systems, but let's keep it raw here and let the camera handle inversion if needed.
    // Actually, standard GLFW is top-left origin.
    // Let's invert Y delta here to match typical camera expectations (up is positive) if needed, 
    // but usually it's better to keep raw delta and let the consumer decide.
    // For now, raw delta.
    // Wait, in InputHandler.cpp: double yOffset = lastY_ - ypos;  // Y inverted in GLFW
    // So if I want positive delta to mean "up", and y increases downwards, then yes:
    // deltaY = lastY - currentY.
    // My implementation: currentY - lastY. So positive delta means moving down.
    // I will leave it as is (standard delta) and let CameraController flip it.
}

} // namespace se
