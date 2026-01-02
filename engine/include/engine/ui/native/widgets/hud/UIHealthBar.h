#pragma once

#include "engine/ui/native/UIControl.h"

#include <functional>

namespace se::ui {

class UIStyleBox;

/**
 * @class UIHealthBar
 * @brief Health bar widget with modern game UI aesthetics.
 *
 * Features:
 * - Rounded corners with configurable radius
 * - Gradient fill based on health percentage
 * - Background, fill, and border styling
 * - HP label display
 * - Smooth animation support
 */
class UIHealthBar : public UIControl {
public:
    using OnHealthChangedCallback = std::function<void(float current, float max)>;

    UIHealthBar();
    ~UIHealthBar() override = default;

    // Health values
    void SetHealth(float current);
    float GetHealth() const { return currentHealth_; }

    void SetMaxHealth(float max);
    float GetMaxHealth() const { return maxHealth_; }

    void SetHealthRange(float current, float max);
    float GetHealthPercent() const;

    // Display options
    void SetShowLabel(bool show) { showLabel_ = show; QueueRedraw(); }
    bool GetShowLabel() const { return showLabel_; }

    void SetLabelFormat(const std::string& format) { labelFormat_ = format; QueueRedraw(); }
    const std::string& GetLabelFormat() const { return labelFormat_; }

    // Styling
    void SetStyleBoxBackground(std::shared_ptr<UIStyleBox> style) { styleBackground_ = style; QueueRedraw(); }
    void SetStyleBoxFill(std::shared_ptr<UIStyleBox> style) { styleFill_ = style; QueueRedraw(); }
    void SetStyleBoxBorder(std::shared_ptr<UIStyleBox> style) { styleBorder_ = style; QueueRedraw(); }

    // Font
    void SetFontSize(float size) { fontSize_ = size; QueueRedraw(); }
    float GetFontSize() const { return fontSize_; }

    void SetFontColor(const glm::vec4& color) { fontColor_ = color; QueueRedraw(); }
    glm::vec4 GetFontColor() const { return fontColor_; }

    // Color gradient based on health percentage
    void SetLowHealthColor(const glm::vec4& color) { lowHealthColor_ = color; QueueRedraw(); }
    void SetHighHealthColor(const glm::vec4& color) { highHealthColor_ = color; QueueRedraw(); }
    void SetUseGradient(bool use) { useGradient_ = use; QueueRedraw(); }

    // Callbacks
    void SetOnHealthChanged(OnHealthChangedCallback callback) { onHealthChanged_ = std::move(callback); }

    // Override
    glm::vec2 GetMinimumSize() const override;
    void Draw() override;

private:
    glm::vec4 CalculateFillColor() const;

private:
    float currentHealth_ = 100.0f;
    float maxHealth_ = 100.0f;

    bool showLabel_ = true;
    std::string labelFormat_ = "HP";

    float fontSize_ = 14.0f;
    glm::vec4 fontColor_{1.0f, 1.0f, 1.0f, 1.0f};

    bool useGradient_ = true;
    glm::vec4 lowHealthColor_{0.9f, 0.2f, 0.2f, 1.0f};
    glm::vec4 highHealthColor_{0.2f, 0.8f, 0.4f, 1.0f};

    std::shared_ptr<UIStyleBox> styleBackground_;
    std::shared_ptr<UIStyleBox> styleFill_;
    std::shared_ptr<UIStyleBox> styleBorder_;

    OnHealthChangedCallback onHealthChanged_;
};

}  // namespace se::ui
