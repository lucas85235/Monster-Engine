#include "engine/input/GamepadManager.h"

#include <GLFW/glfw3.h>
#include <cmath>

#include "engine/Log.h"
#include "engine/events/Events.h"
#include "engine/events/EventBus.h"

namespace se {

static GamepadManager* s_Instance = nullptr;

void GamepadManager::Init(EventBus* eventBus) {
    if (initialized_) {
        SE_LOG_WARN("GamepadManager already initialized");
        return;
    }

    eventBus_    = eventBus;
    s_Instance   = this;
    initialized_ = true;

    glfwSetJoystickCallback(JoystickCallback);

    for (GamepadId id = 0; id < MaxGamepads; ++id) {
        if (glfwJoystickPresent(id) && glfwJoystickIsGamepad(id)) {
            OnGamepadConnected(id);
        }
    }

    int connectedCount = GetConnectedCount();
    SE_LOG_INFO("GamepadManager initialized. {} gamepad(s) connected", connectedCount);
}

void GamepadManager::Shutdown() {
    if (!initialized_) return;

    glfwSetJoystickCallback(nullptr);
    s_Instance   = nullptr;
    initialized_ = false;

    SE_LOG_INFO("GamepadManager shutdown");
}

void GamepadManager::Update() {
    if (!initialized_) return;

    for (GamepadId id = 0; id < MaxGamepads; ++id) {
        auto& data = gamepads_[id];
        data.previous = data.current;

        // Check if gamepad is present and valid
        bool isPresent = glfwJoystickPresent(id) && glfwJoystickIsGamepad(id);

        if (isPresent) {
            // Handle connection if not already connected
            if (!data.current.connected) {
                OnGamepadConnected(id);
            }
            PollGamepad(id);
        } else if (data.current.connected) {
            // Handle disconnection
            OnGamepadDisconnected(id);
        }
    }
}

void GamepadManager::PollGamepad(GamepadId id) {
    if (id < 0 || id >= MaxGamepads) return;

    GLFWgamepadstate state;
    if (glfwGetGamepadState(id, &state) != GLFW_TRUE) {
        if (gamepads_[id].current.connected) {
            OnGamepadDisconnected(id);
        }
        return;
    }

    auto& current = gamepads_[id].current;

    for (int i = 0; i < Gamepad::ButtonCount && i < GLFW_GAMEPAD_BUTTON_LAST + 1; ++i) {
        current.buttons[i] = (state.buttons[i] == GLFW_PRESS);
    }

    for (int i = 0; i < Gamepad::AxisCount && i < GLFW_GAMEPAD_AXIS_LAST + 1; ++i) {
        current.axes[i] = state.axes[i];
    }
}

void GamepadManager::OnGamepadConnected(GamepadId id) {
    if (id < 0 || id >= MaxGamepads) return;

    auto& data           = gamepads_[id];
    data.current         = GamepadState{};
    data.current.connected = true;
    data.current.name    = glfwGetGamepadName(id);
    data.previous        = data.current;

    SE_LOG_INFO("Gamepad {} connected: {}", id, data.current.name ? data.current.name : "Unknown");

    if (eventBus_) {
        eventBus_->Invoke<GamepadConnectedEvent>(id, data.current.name);
    }
}

void GamepadManager::OnGamepadDisconnected(GamepadId id) {
    if (id < 0 || id >= MaxGamepads) return;

    auto& data = gamepads_[id];
    const char* name = data.current.name;

    data.current  = GamepadState{};
    data.previous = GamepadState{};

    SE_LOG_INFO("Gamepad {} disconnected: {}", id, name ? name : "Unknown");

    if (eventBus_) {
        eventBus_->Invoke<GamepadDisconnectedEvent>(id);
    }
}

void GamepadManager::JoystickCallback(int jid, int event) {
    if (!s_Instance || !s_Instance->initialized_) return;

    if (event == GLFW_CONNECTED) {
        if (glfwJoystickIsGamepad(jid)) {
            s_Instance->OnGamepadConnected(jid);
        }
    } else if (event == GLFW_DISCONNECTED) {
        s_Instance->OnGamepadDisconnected(jid);
    }
}

float GamepadManager::ApplyDeadzone(float value) const {
    if (std::abs(value) < deadzone_) {
        return 0.0f;
    }
    float sign = value > 0.0f ? 1.0f : -1.0f;
    return sign * (std::abs(value) - deadzone_) / (1.0f - deadzone_);
}

bool GamepadManager::IsConnected(GamepadId id) const {
    if (id < 0 || id >= MaxGamepads) return false;
    return gamepads_[id].current.connected;
}

int GamepadManager::GetConnectedCount() const {
    int count = 0;
    for (GamepadId id = 0; id < MaxGamepads; ++id) {
        if (gamepads_[id].current.connected) ++count;
    }
    return count;
}

GamepadId GamepadManager::GetFirstConnectedId() const {
    for (GamepadId id = 0; id < MaxGamepads; ++id) {
        if (gamepads_[id].current.connected) return id;
    }
    return -1;
}

const char* GamepadManager::GetName(GamepadId id) const {
    if (id < 0 || id >= MaxGamepads) return nullptr;
    return gamepads_[id].current.name;
}

bool GamepadManager::IsButtonDown(GamepadId id, GamepadButton button) const {
    if (id < 0 || id >= MaxGamepads) return false;
    if (button < 0 || button >= Gamepad::ButtonCount) return false;
    return gamepads_[id].current.buttons[button];
}

bool GamepadManager::IsButtonPressed(GamepadId id, GamepadButton button) const {
    if (id < 0 || id >= MaxGamepads) return false;
    if (button < 0 || button >= Gamepad::ButtonCount) return false;
    return gamepads_[id].current.buttons[button] && !gamepads_[id].previous.buttons[button];
}

bool GamepadManager::IsButtonReleased(GamepadId id, GamepadButton button) const {
    if (id < 0 || id >= MaxGamepads) return false;
    if (button < 0 || button >= Gamepad::ButtonCount) return false;
    return !gamepads_[id].current.buttons[button] && gamepads_[id].previous.buttons[button];
}

float GamepadManager::GetAxis(GamepadId id, GamepadAxis axis) const {
    return ApplyDeadzone(GetAxisRaw(id, axis));
}

float GamepadManager::GetAxisRaw(GamepadId id, GamepadAxis axis) const {
    if (id < 0 || id >= MaxGamepads) return 0.0f;
    if (axis < 0 || axis >= Gamepad::AxisCount) return 0.0f;
    return gamepads_[id].current.axes[axis];
}

bool GamepadManager::IsButtonDown(GamepadButton button) const {
    GamepadId id = GetFirstConnectedId();
    return id >= 0 ? IsButtonDown(id, button) : false;
}

bool GamepadManager::IsButtonPressed(GamepadButton button) const {
    GamepadId id = GetFirstConnectedId();
    return id >= 0 ? IsButtonPressed(id, button) : false;
}

bool GamepadManager::IsButtonReleased(GamepadButton button) const {
    GamepadId id = GetFirstConnectedId();
    return id >= 0 ? IsButtonReleased(id, button) : false;
}

float GamepadManager::GetAxis(GamepadAxis axis) const {
    GamepadId id = GetFirstConnectedId();
    return id >= 0 ? GetAxis(id, axis) : 0.0f;
}

const GamepadState& GamepadManager::GetState(GamepadId id) const {
    static GamepadState empty;
    if (id < 0 || id >= MaxGamepads) return empty;
    return gamepads_[id].current;
}

}  // namespace se
