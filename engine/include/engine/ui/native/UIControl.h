#pragma once

#include "engine/ui/native/UITypes.h"
#include "engine/ui/native/InputEvent.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace se::ui {

// Forward declarations
class UIContainer;
class UITheme;
class UIStyleBox;

/**
 * @class UIControl
 * @brief Base class for all UI elements (equivalent to Godot's Control).
 *
 * UIControl provides:
 * - Anchor-based responsive positioning
 * - Size flags for container layouts
 * - Input handling with mouse filter and focus
 * - Theme-based styling
 * - Hierarchical parent/child management
 */
class UIControl {
public:
    using Ptr = std::unique_ptr<UIControl>;
    using ClickCallback = std::function<void()>;

    UIControl();
    virtual ~UIControl();

    // Prevent copying, allow moving
    UIControl(const UIControl&) = delete;
    UIControl& operator=(const UIControl&) = delete;
    UIControl(UIControl&&) noexcept = default;
    UIControl& operator=(UIControl&&) noexcept = default;

    // ─────────────────────────────────────────────────────────
    // Position & Size
    // ─────────────────────────────────────────────────────────

    void SetPosition(const glm::vec2& pos);
    void SetPosition(float x, float y) { SetPosition({x, y}); }
    glm::vec2 GetPosition() const { return position_; }

    void SetSize(const glm::vec2& size);
    void SetSize(float w, float h) { SetSize({w, h}); }
    glm::vec2 GetSize() const { return size_; }

    void SetRect(const glm::vec2& pos, const glm::vec2& size);
    
    glm::vec2 GetGlobalPosition() const;
    glm::vec2 GetScreenPosition() const { return GetGlobalPosition(); }

    // ─────────────────────────────────────────────────────────
    // Anchor System
    // ─────────────────────────────────────────────────────────

    void SetAnchor(Side side, float value);
    float GetAnchor(Side side) const { return anchors_[side]; }
    void SetAnchorsPreset(LayoutPreset preset, bool keepOffset = false);

    void SetOffset(Side side, float value);
    float GetOffset(Side side) const { return offsets_[side]; }

    void SetLayoutMode(LayoutMode mode) { layoutMode_ = mode; }
    LayoutMode GetLayoutMode() const { return layoutMode_; }

    void SetGrowDirection(GrowDirection h, GrowDirection v);
    GrowDirection GetHGrowDirection() const { return hGrowDirection_; }
    GrowDirection GetVGrowDirection() const { return vGrowDirection_; }

    // ─────────────────────────────────────────────────────────
    // Minimum Size
    // ─────────────────────────────────────────────────────────

    void SetCustomMinimumSize(const glm::vec2& size);
    glm::vec2 GetCustomMinimumSize() const { return customMinimumSize_; }
    
    virtual glm::vec2 GetMinimumSize() const;
    glm::vec2 GetCombinedMinimumSize() const;
    
    void UpdateMinimumSize();

    // ─────────────────────────────────────────────────────────
    // Size Flags (for container layout)
    // ─────────────────────────────────────────────────────────

    void SetHSizeFlags(uint8_t flags) { hSizeFlags_ = flags; OnSizeFlagsChanged(); }
    void SetVSizeFlags(uint8_t flags) { vSizeFlags_ = flags; OnSizeFlagsChanged(); }
    uint8_t GetHSizeFlags() const { return hSizeFlags_; }
    uint8_t GetVSizeFlags() const { return vSizeFlags_; }

    void SetStretchRatio(float ratio) { stretchRatio_ = ratio; }
    float GetStretchRatio() const { return stretchRatio_; }

    // ─────────────────────────────────────────────────────────
    // Transform
    // ─────────────────────────────────────────────────────────

    void SetRotation(float radians) { rotation_ = radians; UpdateTransform(); }
    float GetRotation() const { return rotation_; }

    void SetScale(const glm::vec2& scale) { scale_ = scale; UpdateTransform(); }
    glm::vec2 GetScale() const { return scale_; }

    void SetPivotOffset(const glm::vec2& pivot) { pivotOffset_ = pivot; UpdateTransform(); }
    glm::vec2 GetPivotOffset() const { return pivotOffset_; }

    glm::mat3 GetTransform() const;

    // ─────────────────────────────────────────────────────────
    // Input Handling
    // ─────────────────────────────────────────────────────────

    void SetMouseFilter(MouseFilter filter) { mouseFilter_ = filter; }
    MouseFilter GetMouseFilter() const { return mouseFilter_; }

    void SetFocusMode(FocusMode mode) { focusMode_ = mode; }
    FocusMode GetFocusMode() const { return focusMode_; }

    void GrabFocus();
    void ReleaseFocus();
    bool HasFocus() const { return hasFocus_; }

    void SetCursorShape(CursorShape shape) { cursorShape_ = shape; }
    CursorShape GetCursorShape() const { return cursorShape_; }

    virtual bool HasPoint(const glm::vec2& point) const;
    virtual void OnInput(const InputEvent& inputEvent);

    // ─────────────────────────────────────────────────────────
    // Visibility
    // ─────────────────────────────────────────────────────────

    void SetVisible(bool visible);
    bool IsVisible() const { return visible_; }
    bool IsVisibleInTree() const;

    void Show() { SetVisible(true); }
    void Hide() { SetVisible(false); }

    // ─────────────────────────────────────────────────────────
    // Hierarchy
    // ─────────────────────────────────────────────────────────

    void AddChild(Ptr child);
    void RemoveChild(UIControl* child);
    void Reparent(UIControl* newParent);
    
    UIControl* GetParent() const { return parent_; }
    UIContainer* GetParentContainer() const;
    
    const std::vector<Ptr>& GetChildren() const { return children_; }
    size_t GetChildCount() const { return children_.size(); }
    UIControl* GetChild(size_t index) const;

    // ─────────────────────────────────────────────────────────
    // Theme
    // ─────────────────────────────────────────────────────────

    void SetTheme(std::shared_ptr<UITheme> theme);
    std::shared_ptr<UITheme> GetTheme() const;

    // ─────────────────────────────────────────────────────────
    // Drawing
    // ─────────────────────────────────────────────────────────

    void QueueRedraw();
    bool NeedsRedraw() const { return needsRedraw_; }
    void ClearRedrawFlag() { needsRedraw_ = false; }

    virtual void Draw();

    // ─────────────────────────────────────────────────────────
    // Notifications
    // ─────────────────────────────────────────────────────────

    virtual void OnNotification(ControlNotification notification);

    // ─────────────────────────────────────────────────────────
    // Identification
    // ─────────────────────────────────────────────────────────

    void SetName(const std::string& name) { name_ = name; }
    const std::string& GetName() const { return name_; }

    void SetId(const std::string& id) { id_ = id; }
    const std::string& GetId() const { return id_; }

protected:
    void UpdateTransform();
    void NotifyResized();
    void OnSizeFlagsChanged();
    void PropagateMinimumSizeChanged();

    // Called when parent size changes (for anchor recalculation)
    void OnParentSizeChanged();

    // Internal layout calculation
    void RecalculateFromAnchors();
    glm::vec2 GetParentSize() const;

protected:
    // Identity
    std::string name_;
    std::string id_;

    // Position & Size
    glm::vec2 position_{0.0f, 0.0f};
    glm::vec2 size_{100.0f, 100.0f};
    glm::vec2 customMinimumSize_{0.0f, 0.0f};
    mutable glm::vec2 cachedMinimumSize_{0.0f, 0.0f};
    mutable bool minimumSizeDirty_ = true;

    // Anchors & Offsets (LTRB order)
    float anchors_[SIDE_MAX] = {0.0f, 0.0f, 0.0f, 0.0f};
    float offsets_[SIDE_MAX] = {0.0f, 0.0f, 0.0f, 0.0f};
    LayoutMode layoutMode_ = LayoutMode::POSITION;
    GrowDirection hGrowDirection_ = GrowDirection::END;
    GrowDirection vGrowDirection_ = GrowDirection::END;

    // Size Flags
    uint8_t hSizeFlags_ = SIZE_FILL;
    uint8_t vSizeFlags_ = SIZE_FILL;
    float stretchRatio_ = 1.0f;

    // Transform
    float rotation_ = 0.0f;
    glm::vec2 scale_{1.0f, 1.0f};
    glm::vec2 pivotOffset_{0.0f, 0.0f};
    mutable glm::mat3 cachedTransform_{1.0f};
    mutable bool transformDirty_ = true;

    // Input
    MouseFilter mouseFilter_ = MouseFilter::MOUSE_STOP;
    FocusMode focusMode_ = FocusMode::NO_FOCUS;
    CursorShape cursorShape_ = CursorShape::ARROW;
    bool hasFocus_ = false;

    // Visibility
    bool visible_ = true;

    // Hierarchy
    UIControl* parent_ = nullptr;
    std::vector<Ptr> children_;

    // Theme
    std::shared_ptr<UITheme> theme_;

    // Rendering
    bool needsRedraw_ = true;
};

}  // namespace se::ui
