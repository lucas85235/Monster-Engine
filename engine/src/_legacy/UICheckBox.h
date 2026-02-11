#pragma once

#include "engine/ui/native/UIControl.h"

#include <functional>
#include <string>

namespace se::ui {

class UIStyleBox;

/**
 * @class UICheckBox
 * @brief Toggle checkbox widget with label.
 *
 * Features:
 * - Checked/unchecked state
 * - Optional text label
 * - Click to toggle
 * - Custom checkbox and label styles
 */
class UICheckBox : public UIControl {
public:
    using ToggledCallback = std::function<void(bool)>;
    
    UICheckBox();
    explicit UICheckBox(const std::string& text);
    ~UICheckBox() override = default;
    
    // State
    void SetChecked(bool checked);
    bool IsChecked() const { return checked_; }
    void Toggle() { SetChecked(!checked_); }
    
    // Text
    void SetText(const std::string& text) { text_ = text; UpdateMinimumSize(); QueueRedraw(); }
    const std::string& GetText() const { return text_; }
    
    // Disabled state
    void SetDisabled(bool disabled);
    bool IsDisabled() const { return disabled_; }
    
    // Callback
    void SetOnToggled(ToggledCallback callback) { onToggled_ = std::move(callback); }
    
    // Style
    void SetStyleBoxUnchecked(std::shared_ptr<UIStyleBox> style) { styleUnchecked_ = style; }
    void SetStyleBoxChecked(std::shared_ptr<UIStyleBox> style) { styleChecked_ = style; }
    void SetStyleBoxUncheckedHover(std::shared_ptr<UIStyleBox> style) { styleUncheckedHover_ = style; }
    void SetStyleBoxCheckedHover(std::shared_ptr<UIStyleBox> style) { styleCheckedHover_ = style; }
    void SetStyleBoxDisabled(std::shared_ptr<UIStyleBox> style) { styleDisabled_ = style; }
    
    // Check icon color
    void SetCheckColor(const glm::vec4& color) { checkColor_ = color; QueueRedraw(); }
    glm::vec4 GetCheckColor() const { return checkColor_; }
    
    // Font
    void SetFontSize(float size) { fontSize_ = size; UpdateMinimumSize(); QueueRedraw(); }
    float GetFontSize() const { return fontSize_; }
    void SetFontColor(const glm::vec4& color) { fontColor_ = color; QueueRedraw(); }
    glm::vec4 GetFontColor() const { return fontColor_; }
    
    // Box size
    void SetBoxSize(float size) { boxSize_ = size; UpdateMinimumSize(); QueueRedraw(); }
    float GetBoxSize() const { return boxSize_; }
    
    // Spacing between box and text
    void SetSeparation(float separation) { separation_ = separation; UpdateMinimumSize(); QueueRedraw(); }
    float GetSeparation() const { return separation_; }
    
    // Override
    glm::vec2 GetMinimumSize() const override;
    void Draw() override;
    void OnInput(const InputEvent& inputEvent) override;
    void OnNotification(ControlNotification notification) override;
    
private:
    std::shared_ptr<UIStyleBox> GetCurrentStyleBox() const;
    
private:
    std::string text_;
    bool checked_ = false;
    bool hovered_ = false;
    bool pressed_ = false;
    bool disabled_ = false;
    
    float boxSize_ = 20.0f;
    float separation_ = 8.0f;
    float fontSize_ = 14.0f;
    
    glm::vec4 fontColor_{1.0f, 1.0f, 1.0f, 1.0f};
    glm::vec4 checkColor_{0.3f, 0.8f, 0.4f, 1.0f};
    
    std::shared_ptr<UIStyleBox> styleUnchecked_;
    std::shared_ptr<UIStyleBox> styleChecked_;
    std::shared_ptr<UIStyleBox> styleUncheckedHover_;
    std::shared_ptr<UIStyleBox> styleCheckedHover_;
    std::shared_ptr<UIStyleBox> styleDisabled_;
    
    ToggledCallback onToggled_;
};

}  // namespace se::ui
