#include "ui/HierarchyPanel.h"

#include <imgui.h>

#include "engine/ecs/SimpleComponents.h"

namespace mst {

void HierarchyPanel::Render(se::Scene& scene, SelectionManager& selection) {
    ImGui::Begin("Hierarchy");

    auto view = scene.GetAllEntitiesWith<se::NameComponent>();
    for (auto entityHandle : view) {
        se::Entity entity(entityHandle, &scene);
        RenderEntityNode(entity, selection);
    }

    // Click on empty space to deselect
    if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
        selection.ClearSelection();
    }

    // Right-click on empty space for context menu
    if (ImGui::BeginPopupContextWindow("HierarchyContextMenu", ImGuiPopupFlags_NoOpenOverItems |
                                                                    ImGuiPopupFlags_MouseButtonRight)) {
        if (ImGui::MenuItem("Create Cube")) {
            // Will be handled by layer
        }
        ImGui::EndPopup();
    }

    ImGui::End();
}

void HierarchyPanel::RenderEntityNode(se::Entity entity, SelectionManager& selection) {
    auto& name = entity.GetComponent<se::NameComponent>().Name;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen |
                               ImGuiTreeNodeFlags_SpanAvailWidth;

    if (selection.IsSelected(entity)) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<uintptr_t>(entity.GetID())), flags, "%s",
                      name.c_str());

    if (ImGui::IsItemClicked()) {
        if (ImGui::GetIO().KeyCtrl) {
            selection.ToggleSelection(entity);
        } else {
            selection.Select(entity);
        }
    }

    RenderContextMenu(entity, selection);
}

void HierarchyPanel::RenderContextMenu(se::Entity entity, SelectionManager& selection) {
    if (ImGui::BeginPopupContextItem()) {
        if (!selection.IsSelected(entity)) {
            selection.Select(entity);
        }

        if (ImGui::MenuItem("Delete", "Delete")) {
            wantsDelete_ = true;
        }

        if (ImGui::MenuItem("Duplicate", "Ctrl+D")) {
            wantsDuplicate_ = true;
        }

        ImGui::EndPopup();
    }
}

void HierarchyPanel::ClearActions() {
    wantsDelete_    = false;
    wantsDuplicate_ = false;
}

}  // namespace mst
