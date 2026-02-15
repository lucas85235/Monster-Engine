#include "engine/ui/native/widgets/UIButton.h"
#include "engine/ui/native/theme/UIStyleBox.h"
#include "engine/ui/native/render/UICanvas2D.h"
#include "engine/ui/native/font/UIFontManager.h"
#include "engine/ui/native/font/UIFont.h"
#include "engine/Log.h"

namespace se::ui {

UIButton::UIButton() : UIControl() {
    SetMouseFilter(MouseFilter::MOUSE_STOP);
    SetFocusMode(FocusMode::ALL_FOCUS);
}

UIButton::UIButton(const std::string& text) : UIButton() {
    text_ = text;
}

void UIButton::SetText(const std::string& text) {
    if (text_ != text) {
        text_ = text;
        UpdateMinimumSize();
        QueueRedraw();
    }
}

void UIButton::SetDisabled(bool disabled) {
    if (disabled) {
        state_ = ButtonState::DISABLED;
    } else if (state_ == ButtonState::DISABLED) {
        state_ = ButtonState::NORMAL;
    }
    QueueRedraw();
}

glm::vec2 UIButton::GetMinimumSize() const {
    float charWidth = fontSize_ * 0.6f;
    float lineHeight = fontSize_ * 1.2f;
    
    float textWidth = text_.length() * charWidth;
    float textHeight = lineHeight;
    
    // Add padding (from style box content margins if available)
    float padH = 16.0f;
    float padV = 8.0f;
    
    if (styleNormal_) {
        padH = styleNormal_->GetContentMargin(SIDE_LEFT) + styleNormal_->GetContentMargin(SIDE_RIGHT);
        padV = styleNormal_->GetContentMargin(SIDE_TOP) + styleNormal_->GetContentMargin(SIDE_BOTTOM);
    }
    
    return {textWidth + padH, textHeight + padV};
}

void UIButton::Draw() {
    auto& canvas = UICanvas2D::Get();
    
    glm::vec2 pos = GetGlobalPosition();
    glm::vec2 size = GetSize();
    glm::vec4 rect{pos.x, pos.y, size.x, size.y};
    
    // Draw background
    auto styleBox = GetCurrentStyleBox();
    if (styleBox) {
        styleBox->Draw(rect);
    } else {
        // Default fallback based on state
        glm::vec4 bgColor;
        switch (state_) {
            case ButtonState::NORMAL:
                bgColor = {0.3f, 0.3f, 0.35f, 1.0f};
                break;
            case ButtonState::HOVER:
                bgColor = {0.4f, 0.4f, 0.45f, 1.0f};
                break;
            case ButtonState::PRESSED:
            case ButtonState::HOVER_PRESSED:
                bgColor = {0.2f, 0.4f, 0.6f, 1.0f};
                break;
            case ButtonState::DISABLED:
                bgColor = {0.2f, 0.2f, 0.22f, 0.7f};
                break;
        }
        canvas.DrawRectFilled(rect, bgColor, 4.0f);
    }
    
    // Draw text centered
    if (!text_.empty()) {
        auto& fontMgr = UIFontManager::Get();
        auto font = fontMgr.GetDefaultFont(fontSize_);
        
        glm::vec2 textSize;
        float textVisualHeight;
        if (font) {
            textSize = font->MeasureText(text_);
            // Use ascender for visual centering (height of capital letters)
            // This gives better visual centering than full lineHeight
            textVisualHeight = font->GetAscender();
        } else {
            // Fallback estimate
            textSize.x = text_.length() * fontSize_ * 0.6f;
            textSize.y = fontSize_ * 1.2f;
            textVisualHeight = fontSize_;
        }
        
        glm::vec2 textPos{
            pos.x + (size.x - textSize.x) * 0.5f,
            pos.y + (size.y - textVisualHeight) * 0.5f
        };
        
        glm::vec4 textColor = fontColor_;
        if (state_ == ButtonState::DISABLED) {
            textColor.a *= 0.5f;
        }
        
        canvas.DrawText(text_, textPos, textColor, 0, fontSize_);
    }
    
    UIControl::Draw();
}

void UIButton::OnInput(const InputEvent& inputEvent) {
    if (state_ == ButtonState::DISABLED) return;
    
    if (inputEvent.IsMouseButton()) {
        if (inputEvent.button == MouseButton::LEFT) {
            if (inputEvent.buttonPressed) {
                pressed_ = true;
                UpdateState(hovered_, pressed_);
                
                if (onPressed_) {
                    onPressed_();
                }
            } else {
                if (pressed_ && hovered_) {
                    // Click completed
                    if (toggleMode_) {
                        toggled_ = !toggled_;
                    }
                    
                    if (onReleased_) {
                        onReleased_();
                    }
                }
                pressed_ = false;
                UpdateState(hovered_, pressed_);
            }
            inputEvent.Accept();
        }
    }
    
    UIControl::OnInput(inputEvent);
}

void UIButton::OnNotification(ControlNotification notification) {
    switch (notification) {
        case ControlNotification::MOUSE_ENTER:
            hovered_ = true;
            UpdateState(hovered_, pressed_);
            break;
            
        case ControlNotification::MOUSE_EXIT:
            hovered_ = false;
            pressed_ = false;
            UpdateState(hovered_, pressed_);
            break;
            
        default:
            break;
    }
    
    UIControl::OnNotification(notification);
}

void UIButton::UpdateState(bool hovered, bool pressed) {
    if (state_ == ButtonState::DISABLED) return;
    
    if (pressed) {
        state_ = hovered ? ButtonState::HOVER_PRESSED : ButtonState::PRESSED;
    } else if (hovered) {
        state_ = ButtonState::HOVER;
    } else {
        state_ = ButtonState::NORMAL;
    }
    
    QueueRedraw();
}

std::shared_ptr<UIStyleBox> UIButton::GetCurrentStyleBox() const {
    switch (state_) {
        case ButtonState::PRESSED:
        case ButtonState::HOVER_PRESSED:
            return stylePressed_ ? stylePressed_ : styleNormal_;
            
        case ButtonState::HOVER:
            return styleHover_ ? styleHover_ : styleNormal_;
            
        case ButtonState::DISABLED:
            return styleDisabled_ ? styleDisabled_ : styleNormal_;
            
        case ButtonState::NORMAL:
        default:
            return styleNormal_;
    }
}

}  // namespace se::ui
