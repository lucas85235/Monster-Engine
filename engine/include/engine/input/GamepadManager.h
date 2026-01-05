#pragma once

#include <array>
#include <functional>
#include <string>

#include "engine/input/GamepadCodes.h"

namespace se {

class EventBus;

class GamepadManager {
   public:
    static GamepadManager& Get() {
        static GamepadManager instance;
        return instance;
    }

    void Init(EventBus* eventBus = nullptr);
    void Shutdown();
    void Update();

    // Deadzone configuration
    void  SetDeadzone(float deadzone) { deadzone_ = deadzone; }
    float GetDeadzone() const { return deadzone_; }

    // Connection state
    bool        IsConnected(GamepadId id) const;
    int         GetConnectedCount() const;
    GamepadId   GetFirstConnectedId() const;
    const char* GetName(GamepadId id) const;

    // Button queries
    bool IsButtonDown(GamepadId id, GamepadButton button) const;
    bool IsButtonPressed(GamepadId id, GamepadButton button) const;
    bool IsButtonReleased(GamepadId id, GamepadButton button) const;

    // Axis queries (returns -1.0 to 1.0 for sticks, 0.0 to 1.0 for triggers)
    float GetAxis(GamepadId id, GamepadAxis axis) const;
    float GetAxisRaw(GamepadId id, GamepadAxis axis) const;

    // Convenience methods for primary gamepad
    bool  IsButtonDown(GamepadButton button) const;
    bool  IsButtonPressed(GamepadButton button) const;
    bool  IsButtonReleased(GamepadButton button) const;
    float GetAxis(GamepadAxis axis) const;

    // Full state access
    const GamepadState& GetState(GamepadId id) const;

    // GLFW callback (called from Window)
    static void JoystickCallback(int jid, int event);

   private:
    GamepadManager() = default;

    float ApplyDeadzone(float value) const;
    void  PollGamepad(GamepadId id);
    void  OnGamepadConnected(GamepadId id);
    void  OnGamepadDisconnected(GamepadId id);

    struct GamepadData {
        GamepadState current;
        GamepadState previous;
    };

    std::array<GamepadData, MaxGamepads> gamepads_{};
    float                                 deadzone_  = DefaultDeadzone;
    EventBus*                             eventBus_  = nullptr;
    bool                                  initialized_ = false;
};

}  // namespace se
