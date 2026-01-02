#include "engine/ui/native/widgets/hud/UIAbilitySlot.h"
#include "engine/ui/native/theme/UIStyleBox.h"
#include "engine/ui/native/render/UICanvas2D.h"

#include <algorithm>

namespace se::ui {

UIAbilitySlot::UIAbilitySlot() {
    SetName("UIAbilitySlot");
    SetCustomMinimumSize({56.0f, 56.0f});
    SetMouseFilter(MouseFilter::MOUSE_STOP);

    // Default background style - frosted glass effect
    auto bg = std::make_shared<UIStyleBoxFlat>();
    bg->SetBackgroundColor({0.15f, 0.15f, 0.2f, 0.85f});
    bg->SetCornerRadiusAll(10.0f);
    bg->SetBorderWidthAll(2.0f);
    bg->SetBorderColor({0.3f, 0.35f, 0.45f, 1.0f});
    styleBackground_ = bg;

    // Active state style - highlighted
    auto active = std::make_shared<UIStyleBoxFlat>();
    active->SetBackgroundColor({0.25f, 0.35f, 0.5f, 0.9f});
    active->SetCornerRadiusAll(10.0f);
    active->SetBorderWidthAll(2.0f);
    active->SetBorderColor({0.5f, 0.6f, 0.8f, 1.0f});
    styleActive_ = active;
}

UIAbilitySlot::UIAbilitySlot(const std::string& keyLabel)
    : UIAbilitySlot() {
    keyLabel_ = keyLabel;
}

void UIAbilitySlot::SetCooldownPercent(float percent) {
    cooldownPercent_ = std::clamp(percent, 0.0f, 1.0f);
    QueueRedraw();
}

glm::vec2 UIAbilitySlot::GetMinimumSize() const {
    glm::vec2 minSize = GetCustomMinimumSize();
    
    // Account for key label at bottom
    if (!keyLabel_.empty()) {
        float labelHeight = keyLabelFontSize_ + 8.0f;
        minSize.y = std::max(minSize.y, minSize.x + labelHeight);
    }
    
    return minSize;
}

void UIAbilitySlot::Draw() {
    auto& canvas = UICanvas2D::Get();
    glm::vec2 pos = GetGlobalPosition();
    glm::vec2 size = GetSize();

    // Reserve space for key label
    float keyLabelHeight = keyLabel_.empty() ? 0.0f : (keyLabelFontSize_ + 6.0f);
    float slotSize = std::min(size.x, size.y - keyLabelHeight);
    
    // Center the slot horizontally
    float slotX = pos.x + (size.x - slotSize) * 0.5f;
    float slotY = pos.y;

    // Choose style based on state
    std::shared_ptr<UIStyleBox> currentStyle = 
        (isActive_ || isPressed_) ? styleActive_ : styleBackground_;

    // Draw background
    if (currentStyle) {
        currentStyle->Draw({slotX, slotY, slotSize, slotSize});
    }

    // Draw icon or placeholder
    float iconPadding = 8.0f;
    float iconSize = slotSize - iconPadding * 2.0f;
    float iconX = slotX + iconPadding;
    float iconY = slotY + iconPadding;

    if (iconTexture_ != 0) {
        canvas.DrawTexture(iconTexture_, 
            {iconX, iconY, iconSize, iconSize},
            {0.0f, 0.0f, 1.0f, 1.0f},
            iconColor_);
    } else {
        // Draw placeholder character
        std::string placeholder(1, placeholderChar_);
        float placeholderSize = iconSize * 0.6f;
        float textX = slotX + (slotSize - placeholderSize * 0.6f) * 0.5f;
        float textY = slotY + (slotSize - placeholderSize) * 0.5f;
        
        glm::vec4 placeholderColor = isEnabled_ 
            ? glm::vec4{0.5f, 0.5f, 0.6f, 0.8f}
            : glm::vec4{0.3f, 0.3f, 0.35f, 0.5f};
        
        canvas.DrawText(placeholder, {textX, textY}, placeholderColor, 0, placeholderSize);
    }

    // Draw cooldown overlay
    if (cooldownPercent_ > 0.0f) {
        DrawCooldownOverlay({slotX, slotY}, {slotSize, slotSize});
    }

    // Draw key label below slot
    if (!keyLabel_.empty()) {
        DrawKeyLabel({slotX, slotY + slotSize}, {slotSize, keyLabelHeight});
    }

    // Draw hover highlight
    if (isHovered_ && !isPressed_) {
        canvas.DrawRect(
            {slotX, slotY, slotSize, slotSize},
            {1.0f, 1.0f, 1.0f, 0.15f},
            2.0f);
    }

    // Draw disabled overlay
    if (!isEnabled_) {
        canvas.DrawRectFilled(
            {slotX, slotY, slotSize, slotSize},
            {0.0f, 0.0f, 0.0f, 0.5f},
            10.0f);
    }
}

void UIAbilitySlot::DrawCooldownOverlay(const glm::vec2& pos, const glm::vec2& size) {
    auto& canvas = UICanvas2D::Get();
    
    // Simple top-down sweep overlay
    float cooldownHeight = size.y * cooldownPercent_;
    
    canvas.DrawRectFilled(
        {pos.x, pos.y, size.x, cooldownHeight},
        cooldownColor_,
        10.0f);
}

void UIAbilitySlot::DrawKeyLabel(const glm::vec2& pos, const glm::vec2& size) {
    auto& canvas = UICanvas2D::Get();

    // Draw small background for key label
    float labelBgWidth = std::min(size.x * 0.7f, static_cast<float>(keyLabel_.length()) * keyLabelFontSize_ * 0.7f + 12.0f);
    float labelBgHeight = keyLabelFontSize_ + 4.0f;
    float labelBgX = pos.x + (size.x - labelBgWidth) * 0.5f;
    float labelBgY = pos.y + 2.0f;

    canvas.DrawRectFilled(
        {labelBgX, labelBgY, labelBgWidth, labelBgHeight},
        {0.1f, 0.1f, 0.15f, 0.9f},
        4.0f);

    // Draw key text centered
    float textWidth = static_cast<float>(keyLabel_.length()) * keyLabelFontSize_ * 0.6f;
    float textX = labelBgX + (labelBgWidth - textWidth) * 0.5f;
    float textY = labelBgY + (labelBgHeight - keyLabelFontSize_) * 0.5f;

    canvas.DrawText(keyLabel_, {textX, textY}, keyLabelColor_, 0, keyLabelFontSize_);
}

void UIAbilitySlot::OnInput(const InputEvent& inputEvent) {
    if (!isEnabled_) return;

    if (inputEvent.IsMouseButton()) {
        if (inputEvent.button == MouseButton::LEFT) {
            if (inputEvent.buttonPressed) {
                isPressed_ = true;
            } else {
                if (isPressed_ && isHovered_) {
                    if (onClick_) {
                        onClick_();
                    }
                }
                isPressed_ = false;
            }
            QueueRedraw();
            inputEvent.Accept();
        }
    }
    
    UIControl::OnInput(inputEvent);
}

void UIAbilitySlot::OnNotification(ControlNotification notification) {
    switch (notification) {
        case ControlNotification::MOUSE_ENTER:
            isHovered_ = true;
            QueueRedraw();
            break;
            
        case ControlNotification::MOUSE_EXIT:
            isHovered_ = false;
            isPressed_ = false;
            QueueRedraw();
            break;
            
        default:
            break;
    }
    
    UIControl::OnNotification(notification);
}

}  // namespace se::ui
