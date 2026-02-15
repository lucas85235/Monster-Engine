#pragma once

#include <filesystem>
#include <glm.hpp>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/input/GamepadCodes.h"
#include "engine/input/KeyCodes.h"
#include "engine/input/MouseCodes.h"

namespace se {

class EventBus;

enum class InputActionType : uint8_t {
    Button = 0,
    Axis   = 1,
};

enum class InputBindingKind : uint8_t {
    Key = 0,
    MouseButton,
    MouseAxisX,
    MouseAxisY,
    MouseScrollY,
    GamepadButton,
    GamepadAxis,
};

struct InputBinding {
    InputBindingKind kind = InputBindingKind::Key;

    KeyCode       key           = 0;
    MouseButton   mouseButton   = Mouse::ButtonLeft;
    GamepadButton gamepadButton = Gamepad::A;
    GamepadAxis   gamepadAxis   = Gamepad::LeftX;

    float scale  = 1.0f;
    bool  invert = false;
};

struct InputActionDefinition {
    InputActionType            type = InputActionType::Button;
    std::vector<InputBinding>  bindings;
};

struct InputActionMap {
    std::string                                           name;
    bool                                                  enabled = true;
    bool                                                  consumeLowerPriority = false;
    std::unordered_map<std::string, InputActionDefinition> actions;
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
    CursorMode GetCursorMode() const {
        return cursorMode_;
    }
    void SetInputSuppressed(bool suppressed) {
        inputSuppressed_ = suppressed;
    }
    bool IsInputSuppressed() const {
        return inputSuppressed_;
    }

    // Action map / context API (Unity-style action maps with runtime stack)
    bool CreateActionMap(const std::string& mapName, bool enabled = true,
                         bool consumeLowerPriority = false);
    bool RemoveActionMap(const std::string& mapName);
    bool HasActionMap(const std::string& mapName) const;
    bool SetActionMapEnabled(const std::string& mapName, bool enabled);
    bool IsActionMapEnabled(const std::string& mapName) const;
    bool SetActionMapConsumeLowerPriority(const std::string& mapName, bool consumeLowerPriority);
    bool DoesActionMapConsumeLowerPriority(const std::string& mapName) const;
    bool HasAction(const std::string& mapName, const std::string& actionName) const;

    bool PushContext(const std::string& mapName);
    bool PopContext();
    bool PopContext(const std::string& mapName);
    void ClearContextStack();

    const std::vector<std::string>& GetContextStack() const {
        return contextStack_;
    }

    const std::unordered_map<std::string, InputActionMap>& GetActionMaps() const {
        return actionMaps_;
    }

    // Binding API (map-aware)
    bool BindAction(const std::string& mapName, const std::string& actionName, KeyCode key);
    bool BindActionMouse(const std::string& mapName, const std::string& actionName,
                         MouseButton button);
    bool BindActionGamepad(const std::string& mapName, const std::string& actionName,
                           GamepadButton button);

    bool BindAxis(const std::string& mapName, const std::string& axisName, KeyCode key,
                  float scale = 1.0f);
    bool BindAxisMouse(const std::string& mapName, const std::string& axisName,
                       InputBindingKind axisKind, float scale = 1.0f);
    bool BindAxisGamepad(const std::string& mapName, const std::string& axisName, GamepadAxis axis,
                         float scale = 1.0f, bool invert = false);

    bool UnbindAction(const std::string& mapName, const std::string& actionName);
    bool UnbindAxis(const std::string& mapName, const std::string& axisName);
    bool UnbindKey(const std::string& mapName, KeyCode key);
    bool UnbindMouseButton(const std::string& mapName, MouseButton button);
    bool UnbindGamepadButton(const std::string& mapName, GamepadButton button);

    void ResetBindings();

    // Compatibility API (binds in default map)
    void BindAction(const std::string& name, KeyCode key);
    void BindAxis(const std::string& name, KeyCode key, float scale = 1.0f);
    void UnbindAction(const std::string& name);
    void UnbindAxis(const std::string& name);

    void BindGamepadAction(const std::string& name, GamepadButton button);
    void BindGamepadAxis(const std::string& name, GamepadAxis axis, float scale = 1.0f,
                         bool invert = false);
    void UnbindGamepadAction(const std::string& name);
    void UnbindGamepadAxis(const std::string& name);

    // Query (unified - resolves by active context stack)
    bool  IsActionPressed(const std::string& name) const;
    bool  IsActionJustPressed(const std::string& name) const;
    bool  IsActionJustReleased(const std::string& name) const;
    float GetAxis(const std::string& name) const;

    // Raw Keyboard/Mouse Input
    bool    IsKeyDown(KeyCode key) const;
    bool    IsKeyJustPressed(KeyCode key) const;
    bool    IsKeyJustReleased(KeyCode key) const;
    bool    IsMouseButtonDown(MouseButton button) const;
    bool    IsMouseButtonJustPressed(MouseButton button) const;
    bool    IsMouseButtonJustReleased(MouseButton button) const;
    Vector2 GetMousePosition() const;
    Vector2 GetMouseDelta() const;
    float   GetScrollDelta() const {
        return inputSuppressed_ ? 0.0f : scrollDelta_;
    }

    // Raw Gamepad Input
    bool  IsGamepadButtonDown(GamepadButton button) const;
    bool  IsGamepadButtonPressed(GamepadButton button) const;
    bool  IsGamepadButtonReleased(GamepadButton button) const;
    float GetGamepadAxis(GamepadAxis axis) const;
    bool  IsGamepadConnected(GamepadId id = 0) const;

    // Persistence
    bool SaveBindings(const std::filesystem::path& path) const;
    bool LoadBindings(const std::filesystem::path& path);

    // Event Handling (called by Window/EventBus)
    void OnKeyPressed(KeyCode key, bool isRepeat = false);
    void OnKeyReleased(KeyCode key);
    void OnMouseButtonPressed(MouseButton button);
    void OnMouseButtonReleased(MouseButton button);
    void OnMouseMoved(float x, float y);
    void OnMouseScrolled(float yOffset);
    void OnTextInput(uint32_t codepoint);
    void OnWindowFocusChanged(bool focused);

    std::vector<uint32_t> ConsumeTextInput();

    // Name helpers for command console / serialization
    static std::string KeyToString(KeyCode key);
    static std::string MouseButtonToString(MouseButton button);
    static std::string GamepadButtonToString(GamepadButton button);
    static std::string GamepadAxisToString(GamepadAxis axis);

    static bool TryParseKey(const std::string& token, KeyCode* outKey);
    static bool TryParseMouseButton(const std::string& token, MouseButton* outButton);
    static bool TryParseGamepadButton(const std::string& token, GamepadButton* outButton);
    static bool TryParseGamepadAxis(const std::string& token, GamepadAxis* outAxis);

   private:
    InputManager() = default;

    struct KeyStateData {
        bool isDown       = false;
        bool justPressed  = false;
        bool justReleased = false;
    };

    InputActionDefinition*       EnsureAction(const std::string& mapName,
                                              const std::string& actionName,
                                              InputActionType type);
    const InputActionDefinition* FindAction(const std::string& actionName) const;

    bool AddBinding(const std::string& mapName, const std::string& actionName,
                    InputActionType type, const InputBinding& binding);

    float EvaluateAxisBinding(const InputBinding& binding) const;
    bool  EvaluateButtonBindingDown(const InputBinding& binding) const;
    bool  EvaluateButtonBindingJustPressed(const InputBinding& binding) const;
    bool  EvaluateButtonBindingJustReleased(const InputBinding& binding) const;

    void ResetPerFrameTransitions();
    void ResetAllInputStates();

    bool IsMouseAxisKey(KeyCode key, InputBindingKind* outKind = nullptr) const;

    std::unordered_map<KeyCode, KeyStateData>     keyStates_;
    std::unordered_map<MouseButton, KeyStateData> mouseButtonStates_;

    std::unordered_map<std::string, InputActionMap> actionMaps_;
    std::vector<std::string>                        contextStack_;

    Vector2    mousePosition_{0.0f};
    Vector2    lastMousePosition_{0.0f};
    Vector2    mouseDelta_{0.0f};
    bool       firstMouse_   = true;
    float      scrollDelta_  = 0.0f;
    CursorMode cursorMode_   = CursorMode::Normal;

    std::vector<uint32_t> textInputQueue_;
    bool                  inputSuppressed_ = false;

    std::string defaultMapName_ = "global";
    EventBus*   eventBus_       = nullptr;
};

}  // namespace se
