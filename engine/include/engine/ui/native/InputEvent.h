#pragma once

#include <se_pch.h>
#include <cstdint>

namespace se::ui {

// Input event types
enum class InputEventType : uint8_t {
    INPUT_NONE,
    MOUSE_MOTION,
    MOUSE_BUTTON,
    MOUSE_SCROLL,
    KEY,
    TEXT
};

// Mouse button identifiers
enum class MouseButton : uint8_t {
    BUTTON_NONE = 0,
    LEFT = 1,
    RIGHT = 2,
    MIDDLE = 3,
    WHEEL_UP = 4,
    WHEEL_DOWN = 5,
    BUTTON_6 = 6,
    BUTTON_7 = 7,
    BUTTON_8 = 8
};

// Key modifiers
enum KeyModifier : uint8_t {
    KEY_MOD_NONE  = 0,
    KEY_MOD_SHIFT = 1 << 0,
    KEY_MOD_CTRL  = 1 << 1,
    KEY_MOD_ALT   = 1 << 2,
    KEY_MOD_SUPER = 1 << 3
};

/**
 * @struct InputEvent
 * @brief Unified input event for UI handling
 */
struct InputEvent {
    InputEventType type = InputEventType::INPUT_NONE;
    
    // Mouse state
    glm::vec2 mousePosition{0.0f};
    glm::vec2 mouseDelta{0.0f};
    MouseButton button = MouseButton::BUTTON_NONE;
    bool buttonPressed = false;
    float scrollDelta = 0.0f;
    
    // Keyboard state
    int keyCode = 0;
    int scanCode = 0;
    bool keyPressed = false;
    bool keyRepeat = false;
    
    // Text input (for TEXT events)
    uint32_t codepoint = 0;
    
    // Modifiers (available for all event types)
    uint8_t modifiers = KEY_MOD_NONE;
    
    // Whether the event has been handled (consumed)
    mutable bool consumed = false;
    
    // Mark event as consumed
    void Accept() const { consumed = true; }
    bool IsConsumed() const { return consumed; }
    
    // Modifier helpers
    bool HasShift() const { return modifiers & KEY_MOD_SHIFT; }
    bool HasCtrl() const { return modifiers & KEY_MOD_CTRL; }
    bool HasAlt() const { return modifiers & KEY_MOD_ALT; }
    bool HasSuper() const { return modifiers & KEY_MOD_SUPER; }
    
    // Type helpers
    bool IsMouseMotion() const { return type == InputEventType::MOUSE_MOTION; }
    bool IsMouseButton() const { return type == InputEventType::MOUSE_BUTTON; }
    bool IsMouseScroll() const { return type == InputEventType::MOUSE_SCROLL; }
    bool IsKey() const { return type == InputEventType::KEY; }
    bool IsText() const { return type == InputEventType::TEXT; }
    
    // Factory methods
    static InputEvent MouseMotion(const glm::vec2& pos, const glm::vec2& delta, uint8_t mods = 0) {
        InputEvent e;
        e.type = InputEventType::MOUSE_MOTION;
        e.mousePosition = pos;
        e.mouseDelta = delta;
        e.modifiers = mods;
        return e;
    }
    
    static InputEvent MouseButtonPressed(MouseButton btn, const glm::vec2& pos, uint8_t mods = 0) {
        InputEvent e;
        e.type = InputEventType::MOUSE_BUTTON;
        e.button = btn;
        e.buttonPressed = true;
        e.mousePosition = pos;
        e.modifiers = mods;
        return e;
    }
    
    static InputEvent MouseButtonReleased(MouseButton btn, const glm::vec2& pos, uint8_t mods = 0) {
        InputEvent e;
        e.type = InputEventType::MOUSE_BUTTON;
        e.button = btn;
        e.buttonPressed = false;
        e.mousePosition = pos;
        e.modifiers = mods;
        return e;
    }
    
    static InputEvent MouseScroll(float delta, const glm::vec2& pos, uint8_t mods = 0) {
        InputEvent e;
        e.type = InputEventType::MOUSE_SCROLL;
        e.scrollDelta = delta;
        e.mousePosition = pos;
        e.modifiers = mods;
        return e;
    }
    
    static InputEvent KeyPressed(int key, int scancode, uint8_t mods = 0, bool repeat = false) {
        InputEvent e;
        e.type = InputEventType::KEY;
        e.keyCode = key;
        e.scanCode = scancode;
        e.keyPressed = true;
        e.keyRepeat = repeat;
        e.modifiers = mods;
        return e;
    }
    
    static InputEvent KeyReleased(int key, int scancode, uint8_t mods = 0) {
        InputEvent e;
        e.type = InputEventType::KEY;
        e.keyCode = key;
        e.scanCode = scancode;
        e.keyPressed = false;
        e.modifiers = mods;
        return e;
    }
    
    static InputEvent TextInput(uint32_t codepoint) {
        InputEvent e;
        e.type = InputEventType::TEXT;
        e.codepoint = codepoint;
        return e;
    }
};

}  // namespace se::ui
