#include "HierarchyPanel.h"

#include "core/UIEditorContext.h"

namespace ued {

void HierarchyPanel::Render(UIEditorContext& ctx) {
    if (!IsVisible()) return;
    
    ImGui::Begin(GetName(), nullptr);
    
    ImGui::Text("Document: %s%s", 
        ctx.GetDocument().GetTitle().c_str(),
        ctx.GetDocument().IsDirty() ? "*" : "");
    ImGui::Separator();
    
    UIWidgetNode* root = ctx.GetWidgetTree().GetRoot();
    if (root) {
        RenderNode(ctx, root);
    }
    
    // Context menu for adding widgets
    if (ImGui::BeginPopupContextWindow("HierarchyContext")) {
        if (ImGui::MenuItem("Add Container")) {
            ctx.GetWidgetTree().AddWidget(ctx.GetWidgetTree().GetSelected(), "div");
            ctx.GetDocument().SetDirty();
        }
        if (ImGui::MenuItem("Add Button")) {
            ctx.GetWidgetTree().AddWidget(ctx.GetWidgetTree().GetSelected(), "button");
            ctx.GetDocument().SetDirty();
        }
        if (ImGui::MenuItem("Add Text")) {
            ctx.GetWidgetTree().AddWidget(ctx.GetWidgetTree().GetSelected(), "p");
            ctx.GetDocument().SetDirty();
        }
        ImGui::EndPopup();
    }
    
    ImGui::End();
}

void HierarchyPanel::RenderNode(UIEditorContext& ctx, UIWidgetNode* node) {
    if (!node) return;
    
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    
    if (node->children.empty()) {
        flags |= ImGuiTreeNodeFlags_Leaf;
    }
    
    if (ctx.GetWidgetTree().GetSelected() == node) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    
    // Root is always open
    if (node->type == "body") {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }
    
    std::string label = node->type;
    if (!node->id.empty() && node->id != "root") {
        label += " #" + node->id;
    }
    
    bool nodeOpen = ImGui::TreeNodeEx(node, flags, "%s", label.c_str());
    
    // Handle selection
    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        ctx.GetWidgetTree().SetSelected(node);
    }
    
    // Drag source
    if (node->type != "body" && ImGui::BeginDragDropSource()) {
        ImGui::SetDragDropPayload("HIERARCHY_NODE", &node, sizeof(UIWidgetNode*));
        ImGui::Text("Move %s", label.c_str());
        ImGui::EndDragDropSource();
    }
    
    // Drop target
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("HIERARCHY_NODE")) {
            UIWidgetNode* draggedNode = *(UIWidgetNode**)payload->Data;
            if (draggedNode != node && draggedNode->parent != node) {
                ctx.GetWidgetTree().MoveWidget(draggedNode, node);
                ctx.GetDocument().SetDirty();
            }
        }
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("WIDGET_TYPE")) {
            const char* widgetType = (const char*)payload->Data;
            ctx.GetWidgetTree().AddWidget(node, widgetType);
            ctx.GetDocument().SetDirty();
        }
        ImGui::EndDragDropTarget();
    }
    
    // Context menu
    if (ImGui::BeginPopupContextItem()) {
        if (node->type != "body") {
            if (ImGui::MenuItem("Delete")) {
                ctx.GetWidgetTree().RemoveWidget(node);
                ctx.GetDocument().SetDirty();
                ImGui::EndPopup();
                if (nodeOpen) ImGui::TreePop();
                return;
            }
            ImGui::Separator();
        }
        if (ImGui::MenuItem("Add Child Container")) {
            ctx.GetWidgetTree().AddWidget(node, "div");
            ctx.GetDocument().SetDirty();
        }
        if (ImGui::MenuItem("Add Child Button")) {
            ctx.GetWidgetTree().AddWidget(node, "button");
            ctx.GetDocument().SetDirty();
        }
        if (ImGui::MenuItem("Add Child Text")) {
            ctx.GetWidgetTree().AddWidget(node, "p");
            ctx.GetDocument().SetDirty();
        }
        ImGui::EndPopup();
    }
    
    if (nodeOpen) {
        for (auto& child : node->children) {
            RenderNode(ctx, child.get());
        }
        ImGui::TreePop();
    }
}

}  // namespace ued
