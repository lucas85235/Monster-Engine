#include "engine/ui/native/widgets/hud/UIHealthBar.h"
#include "engine/ui/native/theme/UIStyleBox.h"
#include "engine/ui/native/render/UICanvas2D.h"

#include <algorithm>
#include <sstream>
#include <iomanip>

namespace se::ui {

UIHealthBar::UIHealthBar() {
    SetName("UIHealthBar");
    SetCustomMinimumSize({200.0f, 28.0f});

    // Default background style - dark semi-transparent
    auto bg = std::make_shared<UIStyleBoxFlat>();
    bg->SetBackgroundColor({0.1f, 0.1f, 0.15f, 0.9f});
    bg->SetCornerRadiusAll(8.0f);
    bg->SetBorderWidthAll(2.0f);
    bg->SetBorderColor({0.2f, 0.2f, 0.3f, 1.0f});
    styleBackground_ = bg;

    // Default fill style - will be overridden by gradient
    auto fill = std::make_shared<UIStyleBoxFlat>();
    fill->SetBackgroundColor({0.2f, 0.8f, 0.4f, 1.0f});
    fill->SetCornerRadiusAll(6.0f);
    styleFill_ = fill;
}

void UIHealthBar::SetHealth(float current) {
    float clamped = std::clamp(current, 0.0f, maxHealth_);
    if (currentHealth_ != clamped) {
        currentHealth_ = clamped;
        QueueRedraw();
        if (onHealthChanged_) {
            onHealthChanged_(currentHealth_, maxHealth_);
        }
    }
}

void UIHealthBar::SetMaxHealth(float max) {
    if (max > 0.0f && maxHealth_ != max) {
        maxHealth_ = max;
        currentHealth_ = std::min(currentHealth_, maxHealth_);
        QueueRedraw();
        if (onHealthChanged_) {
            onHealthChanged_(currentHealth_, maxHealth_);
        }
    }
}

void UIHealthBar::SetHealthRange(float current, float max) {
    if (max > 0.0f) {
        maxHealth_ = max;
        currentHealth_ = std::clamp(current, 0.0f, maxHealth_);
        QueueRedraw();
        if (onHealthChanged_) {
            onHealthChanged_(currentHealth_, maxHealth_);
        }
    }
}

float UIHealthBar::GetHealthPercent() const {
    if (maxHealth_ <= 0.0f) return 0.0f;
    return currentHealth_ / maxHealth_;
}

glm::vec2 UIHealthBar::GetMinimumSize() const {
    glm::vec2 minSize = GetCustomMinimumSize();
    
    if (showLabel_) {
        float textHeight = fontSize_ + 8.0f;
        minSize.y = std::max(minSize.y, textHeight);
    }
    
    return minSize;
}

glm::vec4 UIHealthBar::CalculateFillColor() const {
    if (!useGradient_) {
        return highHealthColor_;
    }
    
    float percent = GetHealthPercent();
    return glm::mix(lowHealthColor_, highHealthColor_, percent);
}

void UIHealthBar::Draw() {
    auto& canvas = UICanvas2D::Get();
    glm::vec2 pos = GetGlobalPosition();
    glm::vec2 size = GetSize();
    
    // Draw background
    if (styleBackground_) {
        styleBackground_->Draw({pos.x, pos.y, size.x, size.y});
    }
    
    // Calculate fill dimensions with padding
    float padding = 3.0f;
    float fillMaxWidth = size.x - padding * 2.0f;
    float fillHeight = size.y - padding * 2.0f;
    float fillWidth = fillMaxWidth * GetHealthPercent();
    
    // Draw fill with gradient color
    if (fillWidth > 0.0f) {
        glm::vec4 fillColor = CalculateFillColor();
        
        // Create temporary style for dynamic color
        auto dynamicFill = std::make_shared<UIStyleBoxFlat>();
        dynamicFill->SetBackgroundColor(fillColor);
        dynamicFill->SetCornerRadiusAll(6.0f);
        
        dynamicFill->Draw({pos.x + padding, pos.y + padding, fillWidth, fillHeight});
    }
    
    // Draw label
    if (showLabel_) {
        std::ostringstream ss;
        ss << labelFormat_ << ": " 
           << std::fixed << std::setprecision(0) << currentHealth_ 
           << "/" << std::setprecision(0) << maxHealth_;
        std::string text = ss.str();
        
        // Center text in bar
        float approxTextWidth = static_cast<float>(text.length()) * fontSize_ * 0.55f;
        float textX = pos.x + (size.x - approxTextWidth) * 0.5f;
        float textY = pos.y + (size.y - fontSize_) * 0.5f;
        
        canvas.DrawText(text, {textX, textY}, fontColor_, 0, fontSize_);
    }
}

}  // namespace se::ui
