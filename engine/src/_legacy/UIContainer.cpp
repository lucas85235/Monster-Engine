#include "engine/ui/native/UIContainer.h"
#include "engine/Log.h"

#include <algorithm>

namespace se::ui {

UIContainer::UIContainer() {
    SE_LOG_DEBUG("UIContainer created");
}

UIContainer::~UIContainer() {
    SE_LOG_DEBUG("UIContainer destroyed");
}

// ─────────────────────────────────────────────────────────
// Layout
// ─────────────────────────────────────────────────────────

void UIContainer::QueueSort() {
    if (pendingSort_) return;
    
    pendingSort_ = true;
    // TODO: Register with UI system for deferred processing
    // For now, just mark as pending
}

void UIContainer::SortChildren() {
    if (!pendingSort_) return;
    
    OnNotification(ControlNotification::PRE_SORT_CHILDREN);
    PerformLayout();
    OnNotification(ControlNotification::SORT_CHILDREN);
    
    pendingSort_ = false;
}

// ─────────────────────────────────────────────────────────
// Child Helpers
// ─────────────────────────────────────────────────────────

void UIContainer::FitChildInRect(UIControl* child, const glm::vec2& pos, const glm::vec2& rectSize) {
    if (!child) return;
    
    glm::vec2 minSize = child->GetCombinedMinimumSize();
    glm::vec2 finalPos = pos;
    glm::vec2 finalSize = rectSize;
    
    uint8_t hFlags = child->GetHSizeFlags();
    uint8_t vFlags = child->GetVSizeFlags();
    
    // Horizontal sizing
    if (!(hFlags & SIZE_FILL)) {
        finalSize.x = minSize.x;
        
        if (hFlags & SIZE_SHRINK_END) {
            finalPos.x += rectSize.x - minSize.x;
        } else if (hFlags & SIZE_SHRINK_CENTER) {
            finalPos.x += (rectSize.x - minSize.x) * 0.5f;
        }
        // SIZE_SHRINK_BEGIN is default (no change to position)
    }
    
    // Vertical sizing
    if (!(vFlags & SIZE_FILL)) {
        finalSize.y = minSize.y;
        
        if (vFlags & SIZE_SHRINK_END) {
            finalPos.y += rectSize.y - minSize.y;
        } else if (vFlags & SIZE_SHRINK_CENTER) {
            finalPos.y += (rectSize.y - minSize.y) * 0.5f;
        }
    }
    
    // Apply
    child->SetRect(finalPos, finalSize);
    
    // Reset transform (containers typically override child transforms)
    child->SetRotation(0.0f);
    child->SetScale({1.0f, 1.0f});
}

// ─────────────────────────────────────────────────────────
// Overrides
// ─────────────────────────────────────────────────────────

glm::vec2 UIContainer::GetMinimumSize() const {
    // Base container: minimum size is the max of all children minimums
    // Subclasses will override with proper layout calculations
    glm::vec2 minSize{0.0f, 0.0f};
    
    for (const auto& child : GetChildren()) {
        if (!child->IsVisible()) continue;
        glm::vec2 childMin = child->GetCombinedMinimumSize();
        minSize = {std::max(minSize.x, childMin.x), std::max(minSize.y, childMin.y)};
    }
    
    return minSize;
}

void UIContainer::OnNotification(ControlNotification notification) {
    UIControl::OnNotification(notification);
    
    switch (notification) {
        case ControlNotification::RESIZED:
            QueueSort();
            break;
        default:
            break;
    }
}

// ─────────────────────────────────────────────────────────
// Protected
// ─────────────────────────────────────────────────────────

void UIContainer::PerformLayout() {
    // Base implementation: do nothing, subclasses override
}

UIControl* UIContainer::AsSortableControl(UIControl* node) const {
    if (!node) return nullptr;
    if (!node->IsVisible()) return nullptr;
    return node;
}

void UIContainer::OnChildAdded(UIControl* child) {
    if (!child) return;
    
    // TODO: Connect to child's size_flags_changed, minimum_size_changed, visibility_changed
    UpdateMinimumSize();
    QueueSort();
}

void UIContainer::OnChildRemoved(UIControl* child) {
    UpdateMinimumSize();
    QueueSort();
}

void UIContainer::OnChildMinimumSizeChanged() {
    UpdateMinimumSize();
    QueueSort();
}

}  // namespace se::ui
