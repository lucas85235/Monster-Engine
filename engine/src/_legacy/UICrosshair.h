#pragma once

#include "engine/ui/native/UIControl.h"

namespace se::ui {

/**
 * @class UICrosshair
 * @brief Crosshair widget for HUD - displays a centered cross indicator.
 *
 * Features:
 * - Configurable line length, thickness, and gap
 * - Optional outline for visibility on bright backgrounds
 * - Auto-centers within parent container
 */
class UICrosshair : public UIControl {
public:
    UICrosshair();
    ~UICrosshair() override = default;

    // Appearance
    void SetLineLength(float length) { lineLength_ = length; QueueRedraw(); }
    float GetLineLength() const { return lineLength_; }

    void SetLineThickness(float thickness) { lineThickness_ = thickness; QueueRedraw(); }
    float GetLineThickness() const { return lineThickness_; }

    void SetGap(float gap) { gap_ = gap; QueueRedraw(); }
    float GetGap() const { return gap_; }

    void SetColor(const glm::vec4& color) { color_ = color; QueueRedraw(); }
    glm::vec4 GetColor() const { return color_; }

    void SetOutlineEnabled(bool enabled) { outlineEnabled_ = enabled; QueueRedraw(); }
    bool IsOutlineEnabled() const { return outlineEnabled_; }

    void SetOutlineColor(const glm::vec4& color) { outlineColor_ = color; QueueRedraw(); }
    glm::vec4 GetOutlineColor() const { return outlineColor_; }

    void SetOutlineThickness(float thickness) { outlineThickness_ = thickness; QueueRedraw(); }
    float GetOutlineThickness() const { return outlineThickness_; }

    // Override
    glm::vec2 GetMinimumSize() const override;
    void Draw() override;

private:
    float lineLength_ = 12.0f;
    float lineThickness_ = 2.0f;
    float gap_ = 4.0f;

    glm::vec4 color_{1.0f, 1.0f, 1.0f, 1.0f};

    bool outlineEnabled_ = true;
    glm::vec4 outlineColor_{0.0f, 0.0f, 0.0f, 0.8f};
    float outlineThickness_ = 1.0f;
};

}  // namespace se::ui
