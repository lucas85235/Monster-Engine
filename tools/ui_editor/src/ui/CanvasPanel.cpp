#include "CanvasPanel.h"

#include "core/UIEditorContext.h"
#include "engine/Log.h"

namespace ued {

CanvasPanel::CanvasPanel() {}

CanvasPanel::~CanvasPanel() {}

void CanvasPanel::Render(UIEditorContext& ctx) {
    if (!IsVisible()) return;
    
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin(GetName(), nullptr, ImGuiWindowFlags_MenuBar);
    
    // Menu bar for canvas options
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Show Widget Bounds", nullptr, &showOverlay_);
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }
    
    isHovered_ = ImGui::IsWindowHovered();
    isFocused_ = ImGui::IsWindowFocused();
    
    ImVec2 availSize = ImGui::GetContentRegionAvail();
    if (availSize.x > 0 && availSize.y > 0) {
        canvasSize_ = availSize;
    }
    
    canvasPos_ = ImGui::GetCursorScreenPos();
    
    // Draw dark background for the canvas area
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(canvasPos_, 
        ImVec2(canvasPos_.x + canvasSize_.x, canvasPos_.y + canvasSize_.y), 
        IM_COL32(25, 25, 30, 255));
    ImGui::Dummy(canvasSize_);
    
    // Handle drag and drop from widget palette
    HandleDragDrop(ctx);
    
    // Render widget structure overlay (for editing)
    if (showOverlay_) {
        RenderWidgetOverlay(ctx);
    }
    
    // Handle click selection
    HandleSelection(ctx);
    
    ImGui::End();
    ImGui::PopStyleVar();
}

void CanvasPanel::HandleDragDrop(UIEditorContext& ctx) {
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("WIDGET_TYPE")) {
            const char* widgetType = (const char*)payload->Data;
            ctx.GetWidgetTree().AddWidget(nullptr, widgetType);
            ctx.GetDocument().SetDirty();
            SE_LOG_INFO("Dropped widget type: {}", widgetType);
        }
        ImGui::EndDragDropTarget();
    }
}

void CanvasPanel::HandleSelection(UIEditorContext& ctx) {
    if (!isHovered_) return;
    
    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        ImVec2 mousePos = ImGui::GetMousePos();
        ImVec2 relPos = ImVec2(mousePos.x - canvasPos_.x, mousePos.y - canvasPos_.y);
        
        // Check if click is within canvas bounds
        if (relPos.x >= 0 && relPos.x < canvasSize_.x && relPos.y >= 0 && relPos.y < canvasSize_.y) {
            // TODO: Implement proper hit testing against rendered RmlUI elements
            // For now, deselect on background click
        }
    }
}

void CanvasPanel::RenderWidgetOverlay(UIEditorContext& ctx) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    int yOffset = 0;
    RenderWidgetBoundsRecursive(ctx.GetWidgetTree().GetRoot(), canvasPos_, ctx, yOffset);
}

void CanvasPanel::RenderWidgetBoundsRecursive(const UIWidgetNode* node, ImVec2 offset, UIEditorContext& ctx, int& yOffset) {
    if (!node) return;
    
    // Skip root body element for rendering overlay
    if (node->type != "body") {
        ImVec2 pos = ImVec2(offset.x + 10, offset.y + 10 + yOffset);
        ImVec2 size = ImVec2(canvasSize_.x - 20, 30);
        
        bool isSelected = (ctx.GetWidgetTree().GetSelected() == node);
        
        ImU32 borderColor = isSelected 
            ? IM_COL32(100, 200, 255, 255) 
            : IM_COL32(100, 100, 100, 100);
        
        ImU32 fillColor = isSelected
            ? IM_COL32(100, 200, 255, 30)
            : IM_COL32(0, 0, 0, 0);
        
        ImDrawList* drawList = ImGui::GetWindowDrawList();
        
        if (isSelected) {
            drawList->AddRectFilled(pos, ImVec2(pos.x + size.x, pos.y + size.y), fillColor, 2.0f);
        }
        drawList->AddRect(pos, ImVec2(pos.x + size.x, pos.y + size.y), borderColor, 2.0f, 0, isSelected ? 2.0f : 1.0f);
        
        // Draw label
        std::string label = "<" + node->type + ">";
        if (!node->id.empty() && node->id != "root") {
            label += " #" + node->id;
        }
        drawList->AddText(ImVec2(pos.x + 5, pos.y + 7), IM_COL32(200, 200, 200, isSelected ? 255 : 150), label.c_str());
        
        yOffset += 35;
    }
    
    for (const auto& child : node->children) {
        RenderWidgetBoundsRecursive(child.get(), offset, ctx, yOffset);
    }
}

}  // namespace ued
