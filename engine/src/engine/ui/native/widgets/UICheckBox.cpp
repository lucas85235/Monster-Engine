#include "engine/ui/native/widgets/UICheckBox.h"
#include "engine/ui/native/theme/UIStyleBox.h"
#include "engine/ui/native/render/UICanvas2D.h"

#include <algorithm>

namespace se::ui {

UICheckBox::UICheckBox() {
    SetName("UICheckBox");
    SetFocusMode(FocusMode::ALL_FOCUS);
    SetCustomMinimumSize({boxSize_, boxSize_});
    
    // Default unchecked style
    auto unchecked = std::make_shared<UIStyleBoxFlat>();
    unchecked->SetBackgroundColor({0.15f, 0.15f, 0.2f, 1.0f});
    unchecked->SetBorderWidthAll(2.0f);
    unchecked->SetBorderColor({0.4f, 0.4f, 0.5f, 1.0f});
    unchecked->SetCornerRadiusAll(4.0f);
    styleUnchecked_ = unchecked;
    
    // Default checked style
    auto checked = std::make_shared<UIStyleBoxFlat>();
    checked->SetBackgroundColor({0.2f, 0.5f, 0.3f, 1.0f});
    checked->SetBorderWidthAll(2.0f);
    checked->SetBorderColor({0.3f, 0.7f, 0.4f, 1.0f});
    checked->SetCornerRadiusAll(4.0f);
    styleChecked_ = checked;
    
    // Default unchecked hover style
    auto uncheckedHover = std::make_shared<UIStyleBoxFlat>();
    uncheckedHover->SetBackgroundColor({0.2f, 0.2f, 0.25f, 1.0f});
    uncheckedHover->SetBorderWidthAll(2.0f);
    uncheckedHover->SetBorderColor({0.5f, 0.5f, 0.6f, 1.0f});
    uncheckedHover->SetCornerRadiusAll(4.0f);
    styleUncheckedHover_ = uncheckedHover;
    
    // Default checked hover style
    auto checkedHover = std::make_shared<UIStyleBoxFlat>();
    checkedHover->SetBackgroundColor({0.25f, 0.6f, 0.35f, 1.0f});
    checkedHover->SetBorderWidthAll(2.0f);
    checkedHover->SetBorderColor({0.4f, 0.8f, 0.5f, 1.0f});
    checkedHover->SetCornerRadiusAll(4.0f);
    styleCheckedHover_ = checkedHover;
    
    // Default disabled style
    auto disabled = std::make_shared<UIStyleBoxFlat>();
    disabled->SetBackgroundColor({0.1f, 0.1f, 0.1f, 1.0f});
    disabled->SetBorderWidthAll(1.0f);
    disabled->SetBorderColor({0.2f, 0.2f, 0.2f, 1.0f});
    disabled->SetCornerRadiusAll(4.0f);
    styleDisabled_ = disabled;
}

UICheckBox::UICheckBox(const std::string& text) : UICheckBox() {
    text_ = text;
    UpdateMinimumSize();
}

void UICheckBox::SetChecked(bool checked) {
    if (checked_ != checked) {
        checked_ = checked;
        if (onToggled_) {
            onToggled_(checked_);
        }
        QueueRedraw();
    }
}

void UICheckBox::SetDisabled(bool disabled) {
    if (disabled_ != disabled) {
        disabled_ = disabled;
        QueueRedraw();
    }
}

glm::vec2 UICheckBox::GetMinimumSize() const {
    glm::vec2 minSize = GetCustomMinimumSize();
    
    float width = boxSize_;
    float height = boxSize_;
    
    if (!text_.empty()) {
        // Add text width (approximate)
        float textWidth = static_cast<float>(text_.length()) * fontSize_ * 0.6f;
        width += separation_ + textWidth;
        height = std::max(height, fontSize_ + 4.0f);
    }
    
    minSize.x = std::max(minSize.x, width);
    minSize.y = std::max(minSize.y, height);
    
    return minSize;
}

void UICheckBox::Draw() {
    auto& canvas = UICanvas2D::Get();
    glm::vec2 pos = GetGlobalPosition();
    glm::vec2 size = GetSize();
    
    // Calculate box position (vertically centered)
    float boxY = pos.y + (size.y - boxSize_) * 0.5f;
    float boxCenterX = pos.x + boxSize_ * 0.5f;
    float boxCenterY = boxY + boxSize_ * 0.5f;
    
    // Draw box background
    auto style = GetCurrentStyleBox();
    if (style) {
        style->Draw({pos.x, boxY, boxSize_, boxSize_});
    }
    
    // Draw checkmark if checked
    if (checked_ && !disabled_) {
        // Classic checkmark: short leg down-left to center-bottom, long leg to top-right
        float padding = boxSize_ * 0.22f;
        float left = pos.x + padding;
        float right = pos.x + boxSize_ - padding;
        float top = boxY + padding;
        float bottom = boxY + boxSize_ - padding;
        
        // Checkmark vertices
        glm::vec2 p1 = {left, boxCenterY - padding * 0.2f};  // Left point (middle-left)
        glm::vec2 p2 = {left + (right - left) * 0.35f, bottom};  // Bottom center point
        glm::vec2 p3 = {right, top};  // Top right point
        
        float lineWidth = std::max(2.5f, boxSize_ * 0.14f);
        canvas.DrawLine(p1, p2, checkColor_, lineWidth);
        canvas.DrawLine(p2, p3, checkColor_, lineWidth);
    }
    
    // Draw text label aligned with the box center
    if (!text_.empty()) {
        float textX = pos.x + boxSize_ + separation_;
        // Align text vertically with center of the checkbox box
        // DrawText adds ascender to position.y, so text visual center is at position.y + ascender/2
        // To center with boxCenterY: textY + ascender/2 = boxCenterY => textY = boxCenterY - fontSize/2
        float textY = boxCenterY - fontSize_ * 0.5f;
        
        glm::vec4 color = disabled_ ? glm::vec4{0.5f, 0.5f, 0.5f, 1.0f} : fontColor_;
        canvas.DrawText(text_, {textX, textY}, color, 0, fontSize_);
    }
}

void UICheckBox::OnInput(const InputEvent& inputEvent) {
    if (disabled_) return;
    
    if (inputEvent.IsMouseButton()) {
        if (inputEvent.button == MouseButton::LEFT) {
            if (inputEvent.buttonPressed) {
                pressed_ = true;
                QueueRedraw();
                inputEvent.Accept();
            } else if (pressed_) {
                pressed_ = false;
                Toggle();
                inputEvent.Accept();
            }
        }
    }
}

void UICheckBox::OnNotification(ControlNotification notification) {
    UIControl::OnNotification(notification);
    
    switch (notification) {
        case ControlNotification::MOUSE_ENTER:
            hovered_ = true;
            QueueRedraw();
            break;
        case ControlNotification::MOUSE_EXIT:
            hovered_ = false;
            pressed_ = false;
            QueueRedraw();
            break;
        default:
            break;
    }
}

std::shared_ptr<UIStyleBox> UICheckBox::GetCurrentStyleBox() const {
    if (disabled_) return styleDisabled_;
    
    if (checked_) {
        return hovered_ ? styleCheckedHover_ : styleChecked_;
    } else {
        return hovered_ ? styleUncheckedHover_ : styleUnchecked_;
    }
}

}  // namespace se::ui
