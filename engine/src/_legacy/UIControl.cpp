#include "engine/ui/native/UIControl.h"
#include "engine/ui/native/UIContainer.h"
#include "engine/Log.h"

#include <gtc/matrix_transform.hpp>
#include <algorithm>
#include <cmath>

namespace se::ui {

UIControl::UIControl() {
    SE_LOG_DEBUG("UIControl created");
}

UIControl::~UIControl() {
    SE_LOG_DEBUG("UIControl destroyed: {}", name_.empty() ? "(unnamed)" : name_);
}

// ─────────────────────────────────────────────────────────
// Position & Size
// ─────────────────────────────────────────────────────────

void UIControl::SetPosition(const glm::vec2& pos) {
    if (position_ == pos) return;
    position_ = pos;
    UpdateTransform();
    QueueRedraw();
}

void UIControl::SetSize(const glm::vec2& size) {
    glm::vec2 minSize = GetCombinedMinimumSize();
    glm::vec2 newSize{std::max(size.x, minSize.x), std::max(size.y, minSize.y)};
    if (size_ == newSize) return;
    size_ = newSize;
    NotifyResized();
    QueueRedraw();
}

void UIControl::SetRect(const glm::vec2& pos, const glm::vec2& size) {
    SetPosition(pos);
    SetSize(size);
}

glm::vec2 UIControl::GetGlobalPosition() const {
    if (parent_) {
        return parent_->GetGlobalPosition() + position_;
    }
    return position_;
}

// ─────────────────────────────────────────────────────────
// Anchor System
// ─────────────────────────────────────────────────────────

void UIControl::SetAnchor(Side side, float value) {
    if (side >= SIDE_MAX) return;
    anchors_[side] = std::clamp(value, 0.0f, 1.0f);
    if (layoutMode_ == LayoutMode::ANCHORS) {
        RecalculateFromAnchors();
    }
}

void UIControl::SetOffset(Side side, float value) {
    if (side >= SIDE_MAX) return;
    offsets_[side] = value;
    if (layoutMode_ == LayoutMode::ANCHORS) {
        RecalculateFromAnchors();
    }
}

void UIControl::SetAnchorsPreset(LayoutPreset preset, bool keepOffset) {
    // Anchor configurations for each preset: {left, top, right, bottom}
    struct AnchorConfig {
        float l, t, r, b;
    };

    static const AnchorConfig presets[] = {
        {0.0f, 0.0f, 0.0f, 0.0f},     // TOP_LEFT
        {1.0f, 0.0f, 1.0f, 0.0f},     // TOP_RIGHT
        {0.0f, 1.0f, 0.0f, 1.0f},     // BOTTOM_LEFT
        {1.0f, 1.0f, 1.0f, 1.0f},     // BOTTOM_RIGHT
        {0.0f, 0.5f, 0.0f, 0.5f},     // CENTER_LEFT
        {0.5f, 0.0f, 0.5f, 0.0f},     // CENTER_TOP
        {1.0f, 0.5f, 1.0f, 0.5f},     // CENTER_RIGHT
        {0.5f, 1.0f, 0.5f, 1.0f},     // CENTER_BOTTOM
        {0.5f, 0.5f, 0.5f, 0.5f},     // CENTER
        {0.0f, 0.0f, 0.0f, 1.0f},     // LEFT_WIDE
        {0.0f, 0.0f, 1.0f, 0.0f},     // TOP_WIDE
        {1.0f, 0.0f, 1.0f, 1.0f},     // RIGHT_WIDE
        {0.0f, 1.0f, 1.0f, 1.0f},     // BOTTOM_WIDE
        {0.5f, 0.0f, 0.5f, 1.0f},     // VCENTER_WIDE
        {0.0f, 0.5f, 1.0f, 0.5f},     // HCENTER_WIDE
        {0.0f, 0.0f, 1.0f, 1.0f},     // FULL_RECT
    };

    size_t idx = static_cast<size_t>(preset);
    if (idx >= static_cast<size_t>(LayoutPreset::PRESET_MAX)) return;

    const auto& config = presets[idx];
    anchors_[SIDE_LEFT] = config.l;
    anchors_[SIDE_TOP] = config.t;
    anchors_[SIDE_RIGHT] = config.r;
    anchors_[SIDE_BOTTOM] = config.b;

    if (!keepOffset) {
        // Reset offsets based on preset type
        glm::vec2 parentSize = GetParentSize();
        
        // For corner/center presets, center the element
        if (preset <= LayoutPreset::CENTER) {
            float halfW = size_.x * 0.5f;
            float halfH = size_.y * 0.5f;
            offsets_[SIDE_LEFT] = -halfW;
            offsets_[SIDE_TOP] = -halfH;
            offsets_[SIDE_RIGHT] = halfW;
            offsets_[SIDE_BOTTOM] = halfH;
        } else {
            // For wide presets, zero offsets
            offsets_[SIDE_LEFT] = 0.0f;
            offsets_[SIDE_TOP] = 0.0f;
            offsets_[SIDE_RIGHT] = 0.0f;
            offsets_[SIDE_BOTTOM] = 0.0f;
        }
    }

    layoutMode_ = LayoutMode::ANCHORS;
    RecalculateFromAnchors();
    SE_LOG_DEBUG("UIControl '{}' set anchor preset: {}", name_, static_cast<int>(preset));
}

void UIControl::SetGrowDirection(GrowDirection h, GrowDirection v) {
    hGrowDirection_ = h;
    vGrowDirection_ = v;
}

// ─────────────────────────────────────────────────────────
// Minimum Size
// ─────────────────────────────────────────────────────────

void UIControl::SetCustomMinimumSize(const glm::vec2& size) {
    customMinimumSize_ = {std::max(size.x, 0.0f), std::max(size.y, 0.0f)};
    minimumSizeDirty_ = true;
    PropagateMinimumSizeChanged();
}

glm::vec2 UIControl::GetMinimumSize() const {
    return glm::vec2(0.0f, 0.0f);
}

glm::vec2 UIControl::GetCombinedMinimumSize() const {
    if (minimumSizeDirty_) {
        glm::vec2 minSize = GetMinimumSize();
        cachedMinimumSize_ = {std::max(minSize.x, customMinimumSize_.x), std::max(minSize.y, customMinimumSize_.y)};
        minimumSizeDirty_ = false;
    }
    return cachedMinimumSize_;
}

void UIControl::UpdateMinimumSize() {
    minimumSizeDirty_ = true;
    PropagateMinimumSizeChanged();
}

void UIControl::PropagateMinimumSizeChanged() {
    if (parent_) {
        parent_->UpdateMinimumSize();
    }
}

// ─────────────────────────────────────────────────────────
// Size Flags
// ─────────────────────────────────────────────────────────

void UIControl::OnSizeFlagsChanged() {
    if (UIContainer* container = GetParentContainer()) {
        // Container will react to child flag change
    }
}

// ─────────────────────────────────────────────────────────
// Transform
// ─────────────────────────────────────────────────────────

glm::mat3 UIControl::GetTransform() const {
    if (transformDirty_) {
        // Build transform: T(pos) * T(pivot) * R * S * T(-pivot)
        glm::mat3 m{1.0f};
        
        // Translate to position
        m[2][0] = position_.x;
        m[2][1] = position_.y;
        
        if (rotation_ != 0.0f || scale_ != glm::vec2(1.0f)) {
            // Translate to pivot
            m[2][0] += pivotOffset_.x;
            m[2][1] += pivotOffset_.y;
            
            // Rotate
            if (rotation_ != 0.0f) {
                float c = std::cos(rotation_);
                float s = std::sin(rotation_);
                glm::mat3 rot{1.0f};
                rot[0][0] = c;  rot[0][1] = s;
                rot[1][0] = -s; rot[1][1] = c;
                m = m * rot;
            }
            
            // Scale
            if (scale_ != glm::vec2(1.0f)) {
                glm::mat3 scl{1.0f};
                scl[0][0] = scale_.x;
                scl[1][1] = scale_.y;
                m = m * scl;
            }
            
            // Translate back from pivot
            m[2][0] -= pivotOffset_.x;
            m[2][1] -= pivotOffset_.y;
        }
        
        cachedTransform_ = m;
        transformDirty_ = false;
    }
    return cachedTransform_;
}

void UIControl::UpdateTransform() {
    transformDirty_ = true;
    for (auto& child : children_) {
        child->UpdateTransform();
    }
}

// ─────────────────────────────────────────────────────────
// Input Handling
// ─────────────────────────────────────────────────────────

void UIControl::GrabFocus() {
    if (focusMode_ == FocusMode::NO_FOCUS) return;
    
    // TODO: Notify focus manager to update focus chain
    hasFocus_ = true;
    OnNotification(ControlNotification::FOCUS_ENTER);
    QueueRedraw();
}

void UIControl::ReleaseFocus() {
    if (!hasFocus_) return;
    
    hasFocus_ = false;
    OnNotification(ControlNotification::FOCUS_EXIT);
    QueueRedraw();
}

bool UIControl::HasPoint(const glm::vec2& point) const {
    return point.x >= 0 && point.x <= size_.x &&
           point.y >= 0 && point.y <= size_.y;
}

void UIControl::OnInput(const InputEvent& inputEvent) {
    // Base implementation does nothing
}

// ─────────────────────────────────────────────────────────
// Visibility
// ─────────────────────────────────────────────────────────

void UIControl::SetVisible(bool visible) {
    if (visible_ == visible) return;
    visible_ = visible;
    OnNotification(ControlNotification::VISIBILITY_CHANGED);
    QueueRedraw();
}

bool UIControl::IsVisibleInTree() const {
    if (!visible_) return false;
    if (parent_) return parent_->IsVisibleInTree();
    return true;
}

// ─────────────────────────────────────────────────────────
// Hierarchy
// ─────────────────────────────────────────────────────────

void UIControl::AddChild(Ptr child) {
    if (!child) return;
    
    child->parent_ = this;
    children_.push_back(std::move(child));
    
    // Notify child entered tree
    children_.back()->OnNotification(ControlNotification::ENTER_CANVAS);
    
    UpdateMinimumSize();
    QueueRedraw();
}

void UIControl::RemoveChild(UIControl* child) {
    auto it = std::find_if(children_.begin(), children_.end(),
        [child](const Ptr& ptr) { return ptr.get() == child; });
    
    if (it != children_.end()) {
        (*it)->OnNotification(ControlNotification::EXIT_CANVAS);
        (*it)->parent_ = nullptr;
        children_.erase(it);
        
        UpdateMinimumSize();
        QueueRedraw();
    }
}

void UIControl::Reparent(UIControl* newParent) {
    if (parent_ == newParent) return;
    
    if (parent_) {
        // Find ourselves in parent's children
        auto& siblings = parent_->children_;
        auto it = std::find_if(siblings.begin(), siblings.end(),
            [this](const Ptr& ptr) { return ptr.get() == this; });
        
        if (it != siblings.end()) {
            Ptr self = std::move(*it);
            siblings.erase(it);
            parent_->UpdateMinimumSize();
            
            if (newParent) {
                newParent->AddChild(std::move(self));
            }
        }
    }
}

UIContainer* UIControl::GetParentContainer() const {
    return dynamic_cast<UIContainer*>(parent_);
}

UIControl* UIControl::GetChild(size_t index) const {
    if (index >= children_.size()) return nullptr;
    return children_[index].get();
}

// ─────────────────────────────────────────────────────────
// Theme
// ─────────────────────────────────────────────────────────

void UIControl::SetTheme(std::shared_ptr<UITheme> theme) {
    theme_ = std::move(theme);
    OnNotification(ControlNotification::THEME_CHANGED);
    QueueRedraw();
}

std::shared_ptr<UITheme> UIControl::GetTheme() const {
    if (theme_) return theme_;
    if (parent_) return parent_->GetTheme();
    return nullptr;  // TODO: Return default theme from ThemeDB
}

// ─────────────────────────────────────────────────────────
// Drawing
// ─────────────────────────────────────────────────────────

void UIControl::QueueRedraw() {
    needsRedraw_ = true;
    // Propagate up to root for rendering pass
}

void UIControl::Draw() {
    // Base implementation - subclasses override
    OnNotification(ControlNotification::NOTIFICATION_DRAW);
}

// ─────────────────────────────────────────────────────────
// Notifications
// ─────────────────────────────────────────────────────────

void UIControl::OnNotification(ControlNotification notification) {
    // Base implementation - subclasses override for specific behaviors
}

// ─────────────────────────────────────────────────────────
// Internal Layout
// ─────────────────────────────────────────────────────────

void UIControl::NotifyResized() {
    OnNotification(ControlNotification::RESIZED);
    
    // Notify children of parent size change
    for (auto& child : children_) {
        child->OnParentSizeChanged();
    }
}

void UIControl::OnParentSizeChanged() {
    if (layoutMode_ == LayoutMode::ANCHORS) {
        RecalculateFromAnchors();
    }
}

glm::vec2 UIControl::GetParentSize() const {
    if (parent_) {
        return parent_->GetSize();
    }
    // TODO: Return viewport size if root
    return glm::vec2(1920.0f, 1080.0f);
}

void UIControl::RecalculateFromAnchors() {
    glm::vec2 parentSize = GetParentSize();
    
    // Calculate edge positions from anchors + offsets
    float left = parentSize.x * anchors_[SIDE_LEFT] + offsets_[SIDE_LEFT];
    float top = parentSize.y * anchors_[SIDE_TOP] + offsets_[SIDE_TOP];
    float right = parentSize.x * anchors_[SIDE_RIGHT] + offsets_[SIDE_RIGHT];
    float bottom = parentSize.y * anchors_[SIDE_BOTTOM] + offsets_[SIDE_BOTTOM];

    glm::vec2 newPos{left, top};
    glm::vec2 newSize{right - left, bottom - top};

    // Enforce minimum size with grow direction
    glm::vec2 minSize = GetCombinedMinimumSize();
    
    if (newSize.x < minSize.x) {
        float diff = minSize.x - newSize.x;
        switch (hGrowDirection_) {
            case GrowDirection::BEGIN:
                newPos.x -= diff;
                break;
            case GrowDirection::END:
                break;
            case GrowDirection::BOTH:
                newPos.x -= diff * 0.5f;
                break;
        }
        newSize.x = minSize.x;
    }
    
    if (newSize.y < minSize.y) {
        float diff = minSize.y - newSize.y;
        switch (vGrowDirection_) {
            case GrowDirection::BEGIN:
                newPos.y -= diff;
                break;
            case GrowDirection::END:
                break;
            case GrowDirection::BOTH:
                newPos.y -= diff * 0.5f;
                break;
        }
        newSize.y = minSize.y;
    }

    // Apply new position and size
    position_ = newPos;
    size_ = newSize;
    UpdateTransform();
    NotifyResized();
    QueueRedraw();
}

}  // namespace se::ui
