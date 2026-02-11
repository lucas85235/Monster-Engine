#pragma once

#include "engine/ui/native/UIControl.h"

#include <functional>

namespace se::ui {

class UIStyleBox;

/**
 * @class UISlider
 * @brief Slider widget for selecting a value within a range.
 *
 * Features:
 * - Configurable min/max range
 * - Step value for discrete increments
 * - Horizontal or vertical orientation
 * - Customizable track and grabber styles
 */
class UISlider : public UIControl {
public:
    using ValueChangedCallback = std::function<void(float)>;
    
    UISlider(bool vertical = false);
    ~UISlider() override = default;
    
    // Range
    void SetValue(float value);
    float GetValue() const { return value_; }
    
    void SetMinValue(float min) { minValue_ = min; ClampValue(); QueueRedraw(); }
    float GetMinValue() const { return minValue_; }
    
    void SetMaxValue(float max) { maxValue_ = max; ClampValue(); QueueRedraw(); }
    float GetMaxValue() const { return maxValue_; }
    
    void SetRange(float min, float max) { minValue_ = min; maxValue_ = max; ClampValue(); QueueRedraw(); }
    
    void SetStep(float step) { step_ = step; }
    float GetStep() const { return step_; }
    
    // Get normalized value [0,1]
    float GetRatio() const;
    
    // Orientation
    void SetVertical(bool vertical) { vertical_ = vertical; UpdateMinimumSize(); QueueRedraw(); }
    bool IsVertical() const { return vertical_; }
    
    // Callback
    void SetOnValueChanged(ValueChangedCallback callback) { onValueChanged_ = std::move(callback); }
    
    // Style
    void SetStyleBoxTrack(std::shared_ptr<UIStyleBox> style) { styleTrack_ = style; }
    void SetStyleBoxGrabber(std::shared_ptr<UIStyleBox> style) { styleGrabber_ = style; }
    void SetStyleBoxGrabberHighlight(std::shared_ptr<UIStyleBox> style) { styleGrabberHighlight_ = style; }
    
    // Grabber size
    void SetGrabberSize(const glm::vec2& size) { grabberSize_ = size; UpdateMinimumSize(); QueueRedraw(); }
    glm::vec2 GetGrabberSize() const { return grabberSize_; }
    
    // Override
    glm::vec2 GetMinimumSize() const override;
    void Draw() override;
    void OnInput(const InputEvent& inputEvent) override;
    void OnNotification(ControlNotification notification) override;
    
private:
    void ClampValue();
    void UpdateValueFromPosition(const glm::vec2& pos);
    glm::vec4 GetGrabberRect() const;
    
private:
    float value_ = 0.0f;
    float minValue_ = 0.0f;
    float maxValue_ = 100.0f;
    float step_ = 1.0f;
    
    bool vertical_ = false;
    bool dragging_ = false;
    bool hovered_ = false;
    
    glm::vec2 grabberSize_{16.0f, 24.0f};
    
    std::shared_ptr<UIStyleBox> styleTrack_;
    std::shared_ptr<UIStyleBox> styleGrabber_;
    std::shared_ptr<UIStyleBox> styleGrabberHighlight_;
    
    ValueChangedCallback onValueChanged_;
};

/**
 * @class UIHSlider
 * @brief Horizontal slider (convenience alias)
 */
class UIHSlider : public UISlider {
public:
    UIHSlider() : UISlider(false) {}
};

/**
 * @class UIVSlider
 * @brief Vertical slider (convenience alias)
 */
class UIVSlider : public UISlider {
public:
    UIVSlider() : UISlider(true) {}
};

}  // namespace se::ui
