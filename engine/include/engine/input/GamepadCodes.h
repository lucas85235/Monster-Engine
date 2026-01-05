 #pragma once
#include <cstdint>

namespace se {

using GamepadId = int;
using GamepadButton = int;
using GamepadAxis = int;

constexpr int MaxGamepads = 16;
constexpr float DefaultDeadzone = 0.15f;

namespace Gamepad {

enum Button : GamepadButton {
    A             = 0,
    B             = 1,
    X             = 2,
    Y             = 3,
    LeftBumper    = 4,
    RightBumper   = 5,
    Back          = 6,
    Start         = 7,
    Guide         = 8,
    LeftThumb     = 9,
    RightThumb    = 10,
    DPadUp        = 11,
    DPadRight     = 12,
    DPadDown      = 13,
    DPadLeft      = 14,

    // Aliases
    Cross         = A,
    Circle        = B,
    Square        = X,
    Triangle      = Y,
    L1            = LeftBumper,
    R1            = RightBumper,
    L3            = LeftThumb,
    R3            = RightThumb,
    Select        = Back,
    Options       = Start,

    ButtonCount   = 15
};

enum Axis : GamepadAxis {
    LeftX         = 0,
    LeftY         = 1,
    RightX        = 2,
    RightY        = 3,
    LeftTrigger   = 4,
    RightTrigger  = 5,

    // Aliases
    L2            = LeftTrigger,
    R2            = RightTrigger,

    AxisCount     = 6
};

}  // namespace Gamepad

struct GamepadState {
    bool  buttons[Gamepad::ButtonCount] = {};
    float axes[Gamepad::AxisCount]      = {};
    bool  connected                     = false;
    const char* name                    = nullptr;
};

}  // namespace se
