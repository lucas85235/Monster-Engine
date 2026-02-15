#pragma once

#include "engine/ui/native/UIControl.h"

namespace se::ui {

/**
 * @class UIContainer
 * @brief Base class for layout containers (equivalent to Godot's Container).
 *
 * UIContainer provides:
 * - Deferred layout updates via QueueSort()
 * - Child connection/disconnection handling
 * - FitChildInRect() helper for positioning children
 */
class UIContainer : public UIControl {
public:
    UIContainer();
    ~UIContainer() override;

    // ─────────────────────────────────────────────────────────
    // Layout
    // ─────────────────────────────────────────────────────────

    /// Queue a layout update for next frame
    void QueueSort();
    
    /// Force immediate layout update
    void SortChildren();
    
    /// Check if layout is pending
    bool IsSortPending() const { return pendingSort_; }

    // ─────────────────────────────────────────────────────────
    // Child Helpers
    // ─────────────────────────────────────────────────────────

    /// Position a child within a rect, respecting its size flags
    void FitChildInRect(UIControl* child, const glm::vec2& pos, const glm::vec2& size);

    // ─────────────────────────────────────────────────────────
    // Overrides
    // ─────────────────────────────────────────────────────────

    glm::vec2 GetMinimumSize() const override;
    void OnNotification(ControlNotification notification) override;

protected:
    /// Override in subclasses to implement custom layout
    virtual void PerformLayout();
    
    /// Get child as sortable control (visible only)
    UIControl* AsSortableControl(UIControl* node) const;

    void OnChildAdded(UIControl* child);
    void OnChildRemoved(UIControl* child);
    void OnChildMinimumSizeChanged();

private:
    bool pendingSort_ = false;
};

}  // namespace se::ui
