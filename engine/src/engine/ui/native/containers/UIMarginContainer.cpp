#include "engine/ui/native/containers/UIMarginContainer.h"
#include "engine/ui/native/theme/UIStyleBox.h"

#include <algorithm>

namespace se::ui {

// ============================================================
// UIMarginContainer
// ============================================================

UIMarginContainer::UIMarginContainer() {
    SetName("UIMarginContainer");
}

void UIMarginContainer::SetMargin(int side, float margin) {
    if (side >= 0 && side < 4) {
        margins_[side] = margin;
        UpdateMinimumSize();
        QueueSort();
    }
}

void UIMarginContainer::SetMarginAll(float margin) {
    for (int i = 0; i < 4; i++) {
        margins_[i] = margin;
    }
    UpdateMinimumSize();
    QueueSort();
}

void UIMarginContainer::SetMargins(float left, float top, float right, float bottom) {
    margins_[0] = left;
    margins_[1] = top;
    margins_[2] = right;
    margins_[3] = bottom;
    UpdateMinimumSize();
    QueueSort();
}

glm::vec2 UIMarginContainer::GetMinimumSize() const {
    glm::vec2 minSize = GetCustomMinimumSize();
    
    // Add margins
    float marginW = margins_[0] + margins_[2];
    float marginH = margins_[1] + margins_[3];
    
    // Add child's minimum size
    const auto& children = GetChildren();
    if (!children.empty()) {
        UIControl* child = children[0].get();
        if (child && child->IsVisible()) {
            glm::vec2 childMin = child->GetCombinedMinimumSize();
            minSize.x = std::max(minSize.x, childMin.x + marginW);
            minSize.y = std::max(minSize.y, childMin.y + marginH);
        }
    }
    
    minSize.x = std::max(minSize.x, marginW);
    minSize.y = std::max(minSize.y, marginH);
    
    return minSize;
}

void UIMarginContainer::PerformLayout() {
    const auto& children = GetChildren();
    if (children.empty()) return;
    
    UIControl* child = children[0].get();
    if (!child || !child->IsVisible()) return;
    
    glm::vec2 size = GetSize();
    
    // Calculate child position and size within margins
    float childX = margins_[0];
    float childY = margins_[1];
    float childW = size.x - margins_[0] - margins_[2];
    float childH = size.y - margins_[1] - margins_[3];
    
    FitChildInRect(child, {childX, childY}, {childW, childH});
}

// ============================================================
// UICenterContainer
// ============================================================

UICenterContainer::UICenterContainer() {
    SetName("UICenterContainer");
}

glm::vec2 UICenterContainer::GetMinimumSize() const {
    glm::vec2 minSize = GetCustomMinimumSize();
    
    // Use largest child minimum size
    const auto& children = GetChildren();
    if (!children.empty()) {
        UIControl* child = children[0].get();
        if (child && child->IsVisible()) {
            glm::vec2 childMin = child->GetCombinedMinimumSize();
            minSize.x = std::max(minSize.x, childMin.x);
            minSize.y = std::max(minSize.y, childMin.y);
        }
    }
    
    return minSize;
}

void UICenterContainer::PerformLayout() {
    const auto& children = GetChildren();
    if (children.empty()) return;
    
    UIControl* child = children[0].get();
    if (!child || !child->IsVisible()) return;
    
    glm::vec2 size = GetSize();
    glm::vec2 childMin = child->GetCombinedMinimumSize();
    
    // Calculate centered position
    float childX = centerH_ ? (size.x - childMin.x) * 0.5f : 0.0f;
    float childY = centerV_ ? (size.y - childMin.y) * 0.5f : 0.0f;
    
    // Child stays at its minimum size
    child->SetPosition(childX, childY);
    child->SetSize(childMin.x, childMin.y);
}

// ============================================================
// UIPanelContainer
// ============================================================

UIPanelContainer::UIPanelContainer() {
    SetName("UIPanelContainer");
    
    // Default panel style
    auto panel = std::make_shared<UIStyleBoxFlat>();
    panel->SetBackgroundColor({0.1f, 0.1f, 0.15f, 0.95f});
    panel->SetBorderWidthAll(1.0f);
    panel->SetBorderColor({0.3f, 0.3f, 0.4f, 1.0f});
    panel->SetCornerRadiusAll(6.0f);
    panel->SetContentMarginAll(8.0f);
    style_ = panel;
}

void UIPanelContainer::SetStyleBox(std::shared_ptr<UIStyleBox> style) {
    style_ = style;
    UpdateMinimumSize();
    QueueSort();
    QueueRedraw();
}

glm::vec2 UIPanelContainer::GetMinimumSize() const {
    glm::vec2 minSize = GetCustomMinimumSize();
    
    // Get content margins from style
    glm::vec4 margins{0.0f};
    if (style_) {
        margins = style_->GetContentMargins();
    }
    
    float marginW = margins.x + margins.z;  // Left + Right
    float marginH = margins.y + margins.w;  // Top + Bottom
    
    // Add child's minimum size
    const auto& children = GetChildren();
    if (!children.empty()) {
        UIControl* child = children[0].get();
        if (child && child->IsVisible()) {
            glm::vec2 childMin = child->GetCombinedMinimumSize();
            minSize.x = std::max(minSize.x, childMin.x + marginW);
            minSize.y = std::max(minSize.y, childMin.y + marginH);
        }
    }
    
    // Ensure style minimum size
    if (style_) {
        glm::vec2 styleMin = style_->GetMinimumSize();
        minSize.x = std::max(minSize.x, styleMin.x);
        minSize.y = std::max(minSize.y, styleMin.y);
    }
    
    return minSize;
}

void UIPanelContainer::Draw() {
    if (style_) {
        glm::vec2 pos = GetGlobalPosition();
        glm::vec2 size = GetSize();
        style_->Draw({pos.x, pos.y, size.x, size.y});
    }
}

void UIPanelContainer::PerformLayout() {
    const auto& children = GetChildren();
    if (children.empty()) return;
    
    UIControl* child = children[0].get();
    if (!child || !child->IsVisible()) return;
    
    glm::vec2 size = GetSize();
    
    // Get content margins from style
    glm::vec4 margins{0.0f};
    if (style_) {
        margins = style_->GetContentMargins();
    }
    
    // Calculate child position and size within content area
    float childX = margins.x;
    float childY = margins.y;
    float childW = size.x - margins.x - margins.z;
    float childH = size.y - margins.y - margins.w;
    
    FitChildInRect(child, {childX, childY}, {childW, childH});
}

}  // namespace se::ui
