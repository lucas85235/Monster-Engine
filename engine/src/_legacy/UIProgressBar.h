#pragma once

#include "engine/ui/native/UIControl.h"

namespace se::ui {

class UIStyleBox;

/**
 * @class UIProgressBar
 * @brief Progress indicator widget displaying a value between min and max.
 *
 * Features:
 * - Configurable min/max range
 * - Percentage display option
 * - Custom value display format
 * - Filled/background style separation
 */
class UIProgressBar : public UIControl {
public:
    UIProgressBar();
    ~UIProgressBar() override = default;
    
    // Range
    void SetValue(float value);
    float GetValue() const { return value_; }
    
    void SetMinValue(float min) { minValue_ = min; ClampValue(); QueueRedraw(); }
    float GetMinValue() const { return minValue_; }
    
    void SetMaxValue(float max) { maxValue_ = max; ClampValue(); QueueRedraw(); }
    float GetMaxValue() const { return maxValue_; }
    
    void SetRange(float min, float max) { minValue_ = min; maxValue_ = max; ClampValue(); QueueRedraw(); }
    
    // Get normalized value [0,1]
    float GetRatio() const;
    
    // Display
    void SetShowPercentage(bool show) { showPercentage_ = show; QueueRedraw(); }
    bool GetShowPercentage() const { return showPercentage_; }
    
    // Fill direction (false = left-to-right, true = right-to-left)
    void SetFillMode(bool rightToLeft) { rightToLeft_ = rightToLeft; QueueRedraw(); }
    bool GetFillMode() const { return rightToLeft_; }
    
    // Style
    void SetStyleBoxBackground(std::shared_ptr<UIStyleBox> style) { styleBackground_ = style; }
    void SetStyleBoxFill(std::shared_ptr<UIStyleBox> style) { styleFill_ = style; }
    
    // Font
    void SetFontSize(float size) { fontSize_ = size; QueueRedraw(); }
    float GetFontSize() const { return fontSize_; }
    void SetFontColor(const glm::vec4& color) { fontColor_ = color; QueueRedraw(); }
    glm::vec4 GetFontColor() const { return fontColor_; }
    
    // Override
    glm::vec2 GetMinimumSize() const override;
    void Draw() override;
    
private:
    void ClampValue();
    
private:
    float value_ = 0.0f;
    float minValue_ = 0.0f;
    float maxValue_ = 100.0f;
    
    bool showPercentage_ = true;
    bool rightToLeft_ = false;
    
    float fontSize_ = 14.0f;
    glm::vec4 fontColor_{1.0f, 1.0f, 1.0f, 1.0f};
    
    std::shared_ptr<UIStyleBox> styleBackground_;
    std::shared_ptr<UIStyleBox> styleFill_;
};

}  // namespace se::ui
