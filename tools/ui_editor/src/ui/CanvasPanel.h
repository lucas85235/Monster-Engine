#pragma once

#include "IPanel.h"

#include <imgui.h>

#include "Engine.h"

namespace ued {

struct UIWidgetNode;

class CanvasPanel : public IPanel {
public:
    CanvasPanel();
    ~CanvasPanel();
    
    void Render(UIEditorContext& ctx) override;
    const char* GetName() const override { return "Canvas"; }
    
    bool IsHovered() const { return isHovered_; }
    bool IsFocused() const { return isFocused_; }
    
    void ToggleOverlay() { showOverlay_ = !showOverlay_; }
    bool IsOverlayVisible() const { return showOverlay_; }

private:
    void HandleDragDrop(UIEditorContext& ctx);
    void HandleSelection(UIEditorContext& ctx);
    void RenderWidgetOverlay(UIEditorContext& ctx);
    void RenderWidgetBoundsRecursive(const UIWidgetNode* node, ImVec2 offset, UIEditorContext& ctx, int& yOffset);
    
    ImVec2 canvasPos_ = {0, 0};
    ImVec2 canvasSize_ = {800, 600};
    
    bool isHovered_ = false;
    bool isFocused_ = false;
    bool showOverlay_ = true;
    
    float zoom_ = 1.0f;
    ImVec2 pan_ = {0, 0};
};

}  // namespace ued
