#pragma once

#include <sstream>

#include "Event.h"

namespace se {

class KeyEvent : public Event {
   public:
    inline KeyCode GetKeyCode() const {
        return key_code_;
    }

    EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)
   protected:
    KeyEvent(KeyCode keycode) : key_code_(keycode) {}

    KeyCode key_code_;
};

class KeyPressedEvent : public KeyEvent {
   public:
    KeyPressedEvent(KeyCode keycode, int repeatCount) : KeyEvent(keycode), repeat_count_(repeatCount) {}

    inline int GetRepeatCount() const {
        return repeat_count_;
    }

    std::string ToString() const override {
        std::stringstream ss;
        ss << "KeyPressedEvent: " << key_code_ << " (" << repeat_count_ << " repeats)";
        return ss.str();
    }

    EVENT_CLASS_TYPE(KeyPressed)
   private:
    int repeat_count_;
};

class KeyReleasedEvent : public KeyEvent {
   public:
    KeyReleasedEvent(KeyCode keycode) : KeyEvent(keycode) {}

    std::string ToString() const override {
        std::stringstream ss;
        ss << "KeyReleasedEvent: " << key_code_;
        return ss.str();
    }

    EVENT_CLASS_TYPE(KeyReleased)
};

class KeyTypedEvent : public KeyEvent {
   public:
    KeyTypedEvent(KeyCode keycode) : KeyEvent(keycode) {}

    std::string ToString() const override {
        std::stringstream ss;
        ss << "KeyTypedEvent: " << key_code_;
        return ss.str();
    }

    EVENT_CLASS_TYPE(KeyTyped)
};
}  // namespace se