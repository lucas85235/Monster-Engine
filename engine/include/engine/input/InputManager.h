#pragma once

#include <functional>
#include <glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/input/GamepadCodes.h"
#include "engine/input/KeyCodes.h"

namespace se {

class EventBus;

struct ActionBinding {
    std::string Name;
    KeyCode     Key;
};

struct AxisBinding {
    std::string Name;
    KeyCode     Key;
    float       Scale;
};

struct GamepadActionBinding {
    std::string    Name;
    GamepadButton  Button;
};

struct GamepadAxisBinding {
    std::string  Name;
    GamepadAxis  Axis;
    float        Scale;
    bool         Invert;
};

class InputManager {
   public:
    static InputManager& Get() {
        static InputManager instance;
        return instance;
    }

    void Init(EventBus* eventBus = nullptr);
    void Shutdown();
    void Update();
    void SetCursorMode(CursorMode mode);

    // Keyboard/Mouse Binding
    void BindAction(const std::string& name, KeyCode key);
    void BindAxis(const std::string& name, KeyCode key, float scale = 1.0f);
    void UnbindAction(const std::string& name);
    void UnbindAxis(const std::string& name);

    // Gamepad Binding
    void BindGamepadAction(const std::string& name, GamepadButton button);
    void BindGamepadAxis(const std::string& name, GamepadAxis axis, float scale = 1.0f, bool invert = false);
    void UnbindGamepadAction(const std::string& name);
    void UnbindGamepadAxis(const std::string& name);

    // Query (unified - checks keyboard/mouse AND gamepad)
    bool  IsActionPressed(const std::string& name) const;
    bool  IsActionJustPressed(const std::string& name) const;
    bool  IsActionJustReleased(const std::string& name) const;
    float GetAxis(const std::string& name) const;

    // Raw Keyboard/Mouse Input
    bool    IsKeyDown(KeyCode key) const;
    bool    IsMouseButtonDown(MouseButton button) const;
    Vector2 GetMousePosition() const;
    Vector2 GetMouseDelta() const;
    float   GetScrollDelta() const { return scrollDelta_; }

    // Raw Gamepad Input (convenience, delegates to GamepadManager)
    bool  IsGamepadButtonDown(GamepadButton button) const;
    bool  IsGamepadButtonPressed(GamepadButton button) const;
    bool  IsGamepadButtonReleased(GamepadButton button) const;
    float GetGamepadAxis(GamepadAxis axis) const;
    bool  IsGamepadConnected(GamepadId id = 0) const;

    // Event Handling (called by Window/EventBus)
    void OnKeyPressed(KeyCode key);
    void OnKeyReleased(KeyCode key);
    void OnMouseButtonPressed(MouseButton button);
    void OnMouseButtonReleased(MouseButton button);
    void OnMouseMoved(float x, float y);
    void OnMouseScrolled(float yOffset);

   private:
    InputManager() = default;

    struct KeyStateData {
        bool IsDown       = false;
        bool JustPressed  = false;
        bool JustReleased = false;
    };

    std::unordered_map<KeyCode, KeyStateData>     keyStates_;
    std::unordered_map<MouseButton, KeyStateData> mouseButtonStates_;

    std::vector<ActionBinding>        actionBindings_;
    std::vector<AxisBinding>          axisBindings_;
    std::vector<GamepadActionBinding> gamepadActionBindings_;
    std::vector<GamepadAxisBinding>   gamepadAxisBindings_;

    Vector2 mousePosition_{0.0f};
    Vector2 lastMousePosition_{0.0f};
    Vector2 mouseDelta_{0.0f};
    bool    firstMouse_  = true;
    float   scrollDelta_ = 0.0f;

    EventBus* eventBus_ = nullptr;
};

}  // namespace se
