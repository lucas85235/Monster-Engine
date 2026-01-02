#include "engine/ui/native/widgets/UILabel.h"
#include "engine/ui/native/render/UICanvas2D.h"
#include "engine/Log.h"

namespace se::ui {

UILabel::UILabel() : UIControl() {
    SetMouseFilter(MouseFilter::MOUSE_IGNORE);
}

UILabel::UILabel(const std::string& text) : UILabel() {
    text_ = text;
}

void UILabel::SetText(const std::string& text) {
    if (text_ != text) {
        text_ = text;
        UpdateMinimumSize();
        QueueRedraw();
    }
}

glm::vec2 UILabel::GetMinimumSize() const {
    // Estimate text size (proper implementation would use font metrics)
    float charWidth = fontSize_ * 0.6f;
    float lineHeight = fontSize_ * 1.2f;
    
    if (text_.empty()) {
        return {0, lineHeight};
    }
    
    float width = text_.length() * charWidth;
    float height = lineHeight;
    
    return {width, height};
}

void UILabel::Draw() {
    auto& canvas = UICanvas2D::Get();
    
    glm::vec2 pos = GetGlobalPosition();
    glm::vec2 size = GetSize();
    
    // Calculate text position based on alignment
    glm::vec2 textPos = pos;
    
    glm::vec2 textSize = GetMinimumSize();
    
    switch (hAlign_) {
        case HorizontalAlign::LEFT:
            textPos.x = pos.x;
            break;
        case HorizontalAlign::CENTER:
            textPos.x = pos.x + (size.x - textSize.x) * 0.5f;
            break;
        case HorizontalAlign::RIGHT:
            textPos.x = pos.x + size.x - textSize.x;
            break;
    }
    
    switch (vAlign_) {
        case VerticalAlign::TOP:
            textPos.y = pos.y;
            break;
        case VerticalAlign::CENTER:
            textPos.y = pos.y + (size.y - textSize.y) * 0.5f;
            break;
        case VerticalAlign::BOTTOM:
            textPos.y = pos.y + size.y - textSize.y;
            break;
    }
    
    canvas.DrawText(text_, textPos, fontColor_, 0, fontSize_);
    
    UIControl::Draw();
}

}  // namespace se::ui
