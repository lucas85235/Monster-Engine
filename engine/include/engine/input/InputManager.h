#pragma once

#include <unordered_map>
#include <string>
#include <functional>
#include <vector>
#include <glm.hpp>
#include "engine/input/KeyCodes.h"

namespace se {

struct ActionBinding {
    std::string Name;
    KeyCode Key;
    // KeyState TriggerState; // Pressed, Released, Held
};

struct AxisBinding {
    std::string Name;
    KeyCode Key;
    float Scale;
};

class InputManager {
public:
    static InputManager& Get() {
        static InputManager instance;
        return instance;
    }

    void Init();
    void Update();
    void SetCursorMode(CursorMode mode);

    // Binding
    void BindAction(const std::string& name, KeyCode key);
    void BindAxis(const std::string& name, KeyCode key, float scale = 1.0f);
    void UnbindAction(const std::string& name);
    void UnbindAxis(const std::string& name);

    // Query
    bool IsActionPressed(const std::string& name) const;
    bool IsActionJustPressed(const std::string& name) const;
    bool IsActionJustReleased(const std::string& name) const;
    float GetAxis(const std::string& name) const;

    // Raw Input (for internal use or debugging)
    bool IsKeyDown(KeyCode key) const;
    bool IsMouseButtonDown(MouseButton button) const;
    glm::vec2 GetMousePosition() const;
    glm::vec2 GetMouseDelta() const;

    // Event Handling
    void OnKeyPressed(KeyCode key);
    void OnKeyReleased(KeyCode key);
    void OnMouseButtonPressed(MouseButton button);
    void OnMouseButtonReleased(MouseButton button);
    void OnMouseMoved(float x, float y);

private:
    InputManager() = default;

    struct KeyStateData {
        bool IsDown = false;
        bool JustPressed = false;
        bool JustReleased = false;
    };

    std::unordered_map<KeyCode, KeyStateData> keyStates_;
    std::unordered_map<MouseButton, KeyStateData> mouseButtonStates_;
    
    std::vector<ActionBinding> actionBindings_;
    std::vector<AxisBinding> axisBindings_;

    glm::vec2 mousePosition_{0.0f};
    glm::vec2 lastMousePosition_{0.0f};
    glm::vec2 mouseDelta_{0.0f};
    bool firstMouse_ = true;
};

} // namespace se
