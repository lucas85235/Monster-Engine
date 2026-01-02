#include "engine/ui/native/widgets/UISlider.h"
#include "engine/ui/native/theme/UIStyleBox.h"
#include "engine/ui/native/render/UICanvas2D.h"

#include <algorithm>
#include <cmath>

namespace se::ui {

UISlider::UISlider(bool vertical) : vertical_(vertical) {
    SetName("UISlider");
    SetCustomMinimumSize(vertical ? glm::vec2{24.0f, 100.0f} : glm::vec2{100.0f, 24.0f});
    SetFocusMode(FocusMode::ALL_FOCUS);
    
    // Default track style
    auto track = std::make_shared<UIStyleBoxFlat>();
    track->SetBackgroundColor({0.2f, 0.2f, 0.25f, 1.0f});
    track->SetCornerRadiusAll(3.0f);
    styleTrack_ = track;
    
    // Default grabber style
    auto grabber = std::make_shared<UIStyleBoxFlat>();
    grabber->SetBackgroundColor({0.5f, 0.5f, 0.6f, 1.0f});
    grabber->SetCornerRadiusAll(4.0f);
    grabber->SetBorderWidthAll(1.0f);
    grabber->SetBorderColor({0.6f, 0.6f, 0.7f, 1.0f});
    styleGrabber_ = grabber;
    
    // Default highlighted grabber style
    auto grabberHighlight = std::make_shared<UIStyleBoxFlat>();
    grabberHighlight->SetBackgroundColor({0.6f, 0.6f, 0.7f, 1.0f});
    grabberHighlight->SetCornerRadiusAll(4.0f);
    grabberHighlight->SetBorderWidthAll(2.0f);
    grabberHighlight->SetBorderColor({0.4f, 0.7f, 1.0f, 1.0f});
    styleGrabberHighlight_ = grabberHighlight;
}

void UISlider::SetValue(float value) {
    float oldValue = value_;
    value_ = value;
    ClampValue();
    
    if (value_ != oldValue) {
        if (onValueChanged_) {
            onValueChanged_(value_);
        }
        QueueRedraw();
    }
}

float UISlider::GetRatio() const {
    float range = maxValue_ - minValue_;
    if (range <= 0.0f) return 0.0f;
    return (value_ - minValue_) / range;
}

glm::vec2 UISlider::GetMinimumSize() const {
    glm::vec2 minSize = GetCustomMinimumSize();
    
    if (vertical_) {
        minSize.x = std::max(minSize.x, grabberSize_.x);
    } else {
        minSize.y = std::max(minSize.y, grabberSize_.y);
    }
    
    return minSize;
}

void UISlider::Draw() {
    glm::vec2 pos = GetGlobalPosition();
    glm::vec2 size = GetSize();
    
    // Draw track
    if (styleTrack_) {
        if (vertical_) {
            float trackWidth = 6.0f;
            float trackX = pos.x + (size.x - trackWidth) * 0.5f;
            styleTrack_->Draw({trackX, pos.y, trackWidth, size.y});
        } else {
            float trackHeight = 6.0f;
            float trackY = pos.y + (size.y - trackHeight) * 0.5f;
            styleTrack_->Draw({pos.x, trackY, size.x, trackHeight});
        }
    }
    
    // Draw grabber
    glm::vec4 grabberRect = GetGrabberRect();
    auto& grabberStyle = (hovered_ || dragging_) ? styleGrabberHighlight_ : styleGrabber_;
    if (grabberStyle) {
        grabberStyle->Draw(grabberRect);
    }
}

void UISlider::OnInput(const InputEvent& inputEvent) {
    if (inputEvent.IsMouseButton() && inputEvent.buttonPressed) {
        glm::vec2 localPos = inputEvent.mousePosition - GetGlobalPosition();
        glm::vec4 grabberRect = GetGrabberRect();
        glm::vec2 grabberPos = {grabberRect.x, grabberRect.y};
        glm::vec2 grabberSize = {grabberRect.z, grabberRect.w};
        
        // Check if click is on grabber or track
        glm::vec2 size = GetSize();
        if (localPos.x >= 0 && localPos.x < size.x && 
            localPos.y >= 0 && localPos.y < size.y) {
            dragging_ = true;
            UpdateValueFromPosition(inputEvent.mousePosition);
            inputEvent.Accept();
        }
    } else if (inputEvent.IsMouseButton() && !inputEvent.buttonPressed) {
        if (dragging_) {
            dragging_ = false;
            QueueRedraw();
            inputEvent.Accept();
        }
    } else if (inputEvent.IsMouseMotion()) {
        if (dragging_) {
            UpdateValueFromPosition(inputEvent.mousePosition);
            inputEvent.Accept();
        }
    }
}

void UISlider::OnNotification(ControlNotification notification) {
    UIControl::OnNotification(notification);
    
    switch (notification) {
        case ControlNotification::MOUSE_ENTER:
            hovered_ = true;
            QueueRedraw();
            break;
        case ControlNotification::MOUSE_EXIT:
            hovered_ = false;
            QueueRedraw();
            break;
        default:
            break;
    }
}

void UISlider::ClampValue() {
    value_ = std::clamp(value_, minValue_, maxValue_);
    
    // Apply step if set
    if (step_ > 0.0f) {
        float steps = std::round((value_ - minValue_) / step_);
        value_ = minValue_ + steps * step_;
        value_ = std::clamp(value_, minValue_, maxValue_);
    }
}

void UISlider::UpdateValueFromPosition(const glm::vec2& pos) {
    glm::vec2 globalPos = GetGlobalPosition();
    glm::vec2 size = GetSize();
    
    float ratio;
    if (vertical_) {
        float grabberHalfHeight = grabberSize_.y * 0.5f;
        float trackLength = size.y - grabberSize_.y;
        float localY = pos.y - globalPos.y - grabberHalfHeight;
        ratio = 1.0f - std::clamp(localY / trackLength, 0.0f, 1.0f);  // Invert for vertical
    } else {
        float grabberHalfWidth = grabberSize_.x * 0.5f;
        float trackLength = size.x - grabberSize_.x;
        float localX = pos.x - globalPos.x - grabberHalfWidth;
        ratio = std::clamp(localX / trackLength, 0.0f, 1.0f);
    }
    
    float newValue = minValue_ + ratio * (maxValue_ - minValue_);
    SetValue(newValue);
}

glm::vec4 UISlider::GetGrabberRect() const {
    glm::vec2 pos = GetGlobalPosition();
    glm::vec2 size = GetSize();
    float ratio = GetRatio();
    
    if (vertical_) {
        float grabberY = pos.y + (size.y - grabberSize_.y) * (1.0f - ratio);
        float grabberX = pos.x + (size.x - grabberSize_.x) * 0.5f;
        return {grabberX, grabberY, grabberSize_.x, grabberSize_.y};
    } else {
        float grabberX = pos.x + (size.x - grabberSize_.x) * ratio;
        float grabberY = pos.y + (size.y - grabberSize_.y) * 0.5f;
        return {grabberX, grabberY, grabberSize_.x, grabberSize_.y};
    }
}

}  // namespace se::ui
