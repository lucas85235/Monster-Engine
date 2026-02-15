#pragma once

#include "engine/ui/native/UIControl.h"

#include <string>
#include <functional>

namespace se::ui {

class UIStyleBox;

// Button states
enum class ButtonState : uint8_t {
    NORMAL,
    PRESSED,
    HOVER,
    DISABLED,
    HOVER_PRESSED
};

/**
 * @class UIButton
 * @brief Interactive button widget with text and states
 */
class UIButton : public UIControl {
public:
    using PressedCallback = std::function<void()>;
    
    UIButton();
    explicit UIButton(const std::string& text);
    ~UIButton() override = default;
    
    // Text
    void SetText(const std::string& text);
    const std::string& GetText() const { return text_; }
    
    // State
    ButtonState GetState() const { return state_; }
    bool IsPressed() const { return state_ == ButtonState::PRESSED || state_ == ButtonState::HOVER_PRESSED; }
    bool IsHovered() const { return state_ == ButtonState::HOVER || state_ == ButtonState::HOVER_PRESSED; }
    bool IsDisabled() const { return state_ == ButtonState::DISABLED; }
    
    // Disabled state
    void SetDisabled(bool disabled);
    
    // Callbacks
    void SetOnPressed(PressedCallback callback) { onPressed_ = std::move(callback); }
    void SetOnReleased(PressedCallback callback) { onReleased_ = std::move(callback); }
    
    // Toggle mode
    void SetToggleMode(bool enabled) { toggleMode_ = enabled; }
    bool IsToggleMode() const { return toggleMode_; }
    void SetToggled(bool toggled) { toggled_ = toggled; QueueRedraw(); }
    bool IsToggled() const { return toggled_; }
    
    // Style boxes for each state
    void SetStyleBoxNormal(std::shared_ptr<UIStyleBox> style) { styleNormal_ = style; }
    void SetStyleBoxHover(std::shared_ptr<UIStyleBox> style) { styleHover_ = style; }
    void SetStyleBoxPressed(std::shared_ptr<UIStyleBox> style) { stylePressed_ = style; }
    void SetStyleBoxDisabled(std::shared_ptr<UIStyleBox> style) { styleDisabled_ = style; }
    
    // Font
    void SetFontSize(float size) { fontSize_ = size; UpdateMinimumSize(); QueueRedraw(); }
    float GetFontSize() const { return fontSize_; }
    void SetFontColor(const glm::vec4& color) { fontColor_ = color; QueueRedraw(); }
    glm::vec4 GetFontColor() const { return fontColor_; }
    
    // Override
    glm::vec2 GetMinimumSize() const override;
    void Draw() override;
    void OnInput(const InputEvent& inputEvent) override;
    void OnNotification(ControlNotification notification) override;
    
private:
    void UpdateState(bool hovered, bool pressed);
    std::shared_ptr<UIStyleBox> GetCurrentStyleBox() const;
    
private:
    std::string text_;
    ButtonState state_ = ButtonState::NORMAL;
    
    bool toggleMode_ = false;
    bool toggled_ = false;
    bool hovered_ = false;
    bool pressed_ = false;
    
    float fontSize_ = 16.0f;
    glm::vec4 fontColor_{1.0f, 1.0f, 1.0f, 1.0f};
    
    std::shared_ptr<UIStyleBox> styleNormal_;
    std::shared_ptr<UIStyleBox> styleHover_;
    std::shared_ptr<UIStyleBox> stylePressed_;
    std::shared_ptr<UIStyleBox> styleDisabled_;
    
    PressedCallback onPressed_;
    PressedCallback onReleased_;
};

}  // namespace se::ui
