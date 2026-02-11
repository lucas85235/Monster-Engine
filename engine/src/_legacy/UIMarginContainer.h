#pragma once

#include "engine/ui/native/UIContainer.h"

namespace se::ui {

/**
 * @class UIMarginContainer
 * @brief Container that adds margins around its single child.
 *
 * Features:
 * - Configurable margins per side
 * - Single child layout
 */
class UIMarginContainer : public UIContainer {
public:
    UIMarginContainer();
    ~UIMarginContainer() override = default;
    
    // Margins
    void SetMargin(int side, float margin);
    float GetMargin(int side) const { return margins_[side]; }
    
    void SetMarginAll(float margin);
    void SetMargins(float left, float top, float right, float bottom);
    
    // Override
    glm::vec2 GetMinimumSize() const override;
    
protected:
    void PerformLayout() override;
    
private:
    float margins_[4] = {0.0f, 0.0f, 0.0f, 0.0f};  // Left, Top, Right, Bottom
};

/**
 * @class UICenterContainer
 * @brief Container that centers its single child.
 *
 * Features:
 * - Centers child horizontally and/or vertically
 * - Keeps child at its minimum size
 */
class UICenterContainer : public UIContainer {
public:
    UICenterContainer();
    ~UICenterContainer() override = default;
    
    // Centering options
    void SetCenterH(bool center) { centerH_ = center; QueueSort(); }
    bool GetCenterH() const { return centerH_; }
    
    void SetCenterV(bool center) { centerV_ = center; QueueSort(); }
    bool GetCenterV() const { return centerV_; }
    
    // Override
    glm::vec2 GetMinimumSize() const override;
    
protected:
    void PerformLayout() override;
    
private:
    bool centerH_ = true;
    bool centerV_ = true;
};

/**
 * @class UIPanelContainer
 * @brief Container with a styled background panel.
 *
 * Features:
 * - StyleBox background
 * - Content margins from style
 */
class UIPanelContainer : public UIContainer {
public:
    UIPanelContainer();
    ~UIPanelContainer() override = default;
    
    // Style
    void SetStyleBox(std::shared_ptr<UIStyleBox> style);
    std::shared_ptr<UIStyleBox> GetStyleBox() const { return style_; }
    
    // Override
    glm::vec2 GetMinimumSize() const override;
    void Draw() override;
    
protected:
    void PerformLayout() override;
    
private:
    std::shared_ptr<UIStyleBox> style_;
};

}  // namespace se::ui
