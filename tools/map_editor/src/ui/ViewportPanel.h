#pragma once
/**
 * ViewportPanel.h - ImGui viewport window with gizmo integration.
 *
 * Displays the rendered scene, handles viewport resizing, picking
 * input, and gizmo manipulation.
 */

#include "IPanel.h"

#include <imgui.h>

#include "Engine.h"
#include "../editor/MousePicker.h"
#include "../editor/ViewportRenderer.h"

namespace mst {

class ViewportPanel : public IPanel {
public:
    ViewportPanel();
    
    void Render(EditorContext& ctx) override;
    const char* GetName() const override { return "Viewport"; }
    
    bool IsHovered() const { return isHovered_; }
    bool IsFocused() const { return isFocused_; }
    
    ImVec2 GetPosition() const { return position_; }
    ImVec2 GetSize() const { return size_; }
    
    ViewportRenderer& GetRenderer() { return *renderer_; }
    
    void ToggleGrid();
    void ToggleColliderDebug();

private:
    void HandleResize();
    void RenderGizmo(EditorContext& ctx);
    void ProcessMousePicking(EditorContext& ctx);
    
    se::Scope<ViewportRenderer> renderer_;
    MousePicker picker_;
    
    ImVec2 position_ = {0, 0};
    ImVec2 size_ = {0, 0};
    bool isHovered_ = false;
    bool isFocused_ = false;
    bool wasMousePressed_ = false;
};

}  // namespace mst
