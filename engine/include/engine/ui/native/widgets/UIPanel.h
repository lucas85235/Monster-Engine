#pragma once

#include "engine/ui/native/UIControl.h"

namespace se::ui {

class UIStyleBox;

/**
 * @class UIPanel
 * @brief Styled background panel
 */
class UIPanel : public UIControl {
public:
    UIPanel();
    ~UIPanel() override = default;
    
    // Style box override
    void SetStyleBox(std::shared_ptr<UIStyleBox> styleBox);
    std::shared_ptr<UIStyleBox> GetStyleBox() const { return styleBox_; }
    
    // Override
    void Draw() override;
    
private:
    std::shared_ptr<UIStyleBox> styleBox_;
};

}  // namespace se::ui
