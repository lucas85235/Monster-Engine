#include "engine/editor/panels/SceneHierarchyPanel.h"

#include <cstring>
#include <imgui.h>
#include <entt.hpp>

#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/editor/EditorLayer.h"

namespace se {

SceneHierarchyPanel::SceneHierarchyPanel(EditorLayer* editor) : editor_(editor) {}

void SceneHierarchyPanel::OnImGuiRender() {
    ImGui::Begin(GetName(), &open_);

    if (!scene_) {
        ImGui::TextDisabled("No scene loaded");
        ImGui::End();
        return;
    }

    // Search bar
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##search", "Search entities...", searchBuffer_, sizeof(searchBuffer_));

    ImGui::Separator();

    // Entity list
    auto& registry = scene_->GetRegistry();
    auto view = registry.view<NameComponent>();

    for (auto entity : view) {
        auto& name = view.get<NameComponent>(entity);

        // Filter by search
        if (searchBuffer_[0] != '\0') {
            // Simple case-insensitive substring search
            std::string nameLower = name.Name;
            std::string filterLower = searchBuffer_;
            for (auto& c : nameLower) c = static_cast<char>(std::tolower(c));
            for (auto& c : filterLower) c = static_cast<char>(std::tolower(c));
            if (nameLower.find(filterLower) == std::string::npos) continue;
        }

        DrawEntityNode(static_cast<uint32_t>(entity), name.Name.c_str());
    }

    // Click empty space to deselect
    if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
        if (editor_) editor_->SetSelectedEntity(UINT32_MAX);
    }

    // Context menu on empty space
    DrawContextMenu();

    ImGui::End();
}

void SceneHierarchyPanel::DrawEntityNode(uint32_t entityHandle, const char* name) {
    uint32_t selected = editor_ ? editor_->GetSelectedEntity() : UINT32_MAX;
    bool isSelected = (entityHandle == selected);

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanAvailWidth |
        ImGuiTreeNodeFlags_Leaf;  // No children for now

    if (isSelected) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool opened = ImGui::TreeNodeEx(reinterpret_cast<void*>(static_cast<intptr_t>(entityHandle)),
                                     flags, "%s", name);

    if (ImGui::IsItemClicked()) {
        if (editor_) editor_->SetSelectedEntity(entityHandle);
    }

    // Right-click context menu on entity
    if (ImGui::BeginPopupContextItem()) {
        if (ImGui::MenuItem("Delete Entity")) {
            if (scene_) {
                auto& registry = scene_->GetRegistry();
                if (registry.valid(static_cast<entt::entity>(entityHandle))) {
                    Entity e(static_cast<entt::entity>(entityHandle), scene_);
                    scene_->DestroyEntity(e);
                    if (editor_ && editor_->GetSelectedEntity() == entityHandle) {
                        editor_->SetSelectedEntity(UINT32_MAX);
                    }
                }
            }
        }
        ImGui::EndPopup();
    }

    if (opened) {
        ImGui::TreePop();
    }
}

void SceneHierarchyPanel::DrawContextMenu() {
    if (ImGui::BeginPopupContextWindow("HierarchyContext", ImGuiPopupFlags_NoOpenOverItems | ImGuiPopupFlags_MouseButtonRight)) {
        if (scene_) {
            if (ImGui::MenuItem("Create Empty Entity")) {
                scene_->CreateEntity("New Entity");
            }
        }
        ImGui::EndPopup();
    }
}

}  // namespace se
