#include "engine/ui/native/widgets/UIPanel.h"
#include "engine/ui/native/theme/UIStyleBox.h"
#include "engine/ui/native/render/UICanvas2D.h"
#include "engine/Log.h"

namespace se::ui {

UIPanel::UIPanel() : UIControl() {
    SetMouseFilter(MouseFilter::MOUSE_STOP);
}

void UIPanel::SetStyleBox(std::shared_ptr<UIStyleBox> styleBox) {
    styleBox_ = styleBox;
    QueueRedraw();
}

void UIPanel::Draw() {
    auto& canvas = UICanvas2D::Get();
    
    glm::vec2 pos = GetGlobalPosition();
    glm::vec2 size = GetSize();
    glm::vec4 rect{pos.x, pos.y, size.x, size.y};
    
    if (styleBox_) {
        styleBox_->Draw(rect);
    } else {
        // Default fallback: simple gray background
        canvas.DrawRectFilled(rect, {0.2f, 0.2f, 0.25f, 1.0f});
    }
    
    UIControl::Draw();
}

}  // namespace se::ui
