#include "engine/ui/native/widgets/UIProgressBar.h"
#include "engine/ui/native/theme/UIStyleBox.h"
#include "engine/ui/native/render/UICanvas2D.h"

#include <algorithm>
#include <sstream>
#include <iomanip>

namespace se::ui {

UIProgressBar::UIProgressBar() {
    SetName("UIProgressBar");
    SetCustomMinimumSize({100.0f, 24.0f});
    
    // Default background style
    auto bg = std::make_shared<UIStyleBoxFlat>();
    bg->SetBackgroundColor({0.15f, 0.15f, 0.2f, 1.0f});
    bg->SetCornerRadiusAll(4.0f);
    styleBackground_ = bg;
    
    // Default fill style
    auto fill = std::make_shared<UIStyleBoxFlat>();
    fill->SetBackgroundColor({0.3f, 0.6f, 0.9f, 1.0f});
    fill->SetCornerRadiusAll(4.0f);
    styleFill_ = fill;
}

void UIProgressBar::SetValue(float value) {
    value_ = value;
    ClampValue();
    QueueRedraw();
}

float UIProgressBar::GetRatio() const {
    float range = maxValue_ - minValue_;
    if (range <= 0.0f) return 0.0f;
    return (value_ - minValue_) / range;
}

glm::vec2 UIProgressBar::GetMinimumSize() const {
    glm::vec2 minSize = GetCustomMinimumSize();
    
    // Ensure minimum height for text
    if (showPercentage_) {
        float textHeight = fontSize_ + 4.0f;
        minSize.y = std::max(minSize.y, textHeight);
    }
    
    return minSize;
}

void UIProgressBar::Draw() {
    auto& canvas = UICanvas2D::Get();
    glm::vec2 pos = GetGlobalPosition();
    glm::vec2 size = GetSize();
    
    // Draw background
    if (styleBackground_) {
        styleBackground_->Draw({pos.x, pos.y, size.x, size.y});
    }
    
    // Calculate fill width based on ratio
    float ratio = GetRatio();
    float fillWidth = size.x * ratio;
    
    if (fillWidth > 0.0f && styleFill_) {
        float fillX = rightToLeft_ ? (pos.x + size.x - fillWidth) : pos.x;
        styleFill_->Draw({fillX, pos.y, fillWidth, size.y});
    }
    
    // Draw percentage text
    if (showPercentage_) {
        std::ostringstream ss;
        ss << std::fixed << std::setprecision(0) << (ratio * 100.0f) << "%";
        std::string text = ss.str();
        
        // Center text in the progress bar
        // Approximate text width (character count * fontSize * 0.6)
        float approxTextWidth = static_cast<float>(text.length()) * fontSize_ * 0.6f;
        float textX = pos.x + (size.x - approxTextWidth) * 0.5f;
        // Vertically center text: DrawText adds ascender internally
        float textY = pos.y + (size.y - fontSize_) * 0.5f;
        
        canvas.DrawText(text, {textX, textY}, fontColor_, 0, fontSize_);
    }
}

void UIProgressBar::ClampValue() {
    value_ = std::clamp(value_, minValue_, maxValue_);
}

}  // namespace se::ui
