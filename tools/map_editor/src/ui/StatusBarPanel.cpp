#include "StatusBarPanel.h"

#include <imgui.h>

#include "../core/EditorContext.h"

namespace mst {

void StatusBarPanel::Render(EditorContext& ctx) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | 
                             ImGuiWindowFlags_NoResize | 
                             ImGuiWindowFlags_NoMove | 
                             ImGuiWindowFlags_NoScrollbar;
    
    ImGui::Begin(GetName(), nullptr, flags);
    
    ImGui::Text("Entities: %zu", ctx.GetScene().GetEntityCount());
    ImGui::SameLine(); ImGui::Text(" | ");
    
    ImGui::SameLine(); 
    ImGui::Text("Selected: %zu", ctx.GetSelection().GetSelectedEntities().size());
    ImGui::SameLine(); ImGui::Text(" | ");
    
    ImGui::SameLine(); 
    ImGui::Text("Grid: %s", gridVisible_ ? "ON" : "OFF");
    ImGui::SameLine(); ImGui::Text(" | ");
    
    ImGui::SameLine(); 
    ImGui::Text("Colliders: %s", colliderDebugVisible_ ? "ON" : "OFF");
    ImGui::SameLine(); ImGui::Text(" | ");
    
    ImGui::SameLine(); 
    ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
    
    if (ctx.GetDocument().IsDirty()) {
        ImGui::SameLine(); ImGui::Text(" | ");
        ImGui::SameLine();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.0f, 1.0f), "[Modified]");
    }
    
    if (ctx.GetCommandSystem().CanUndo()) {
        ImGui::SameLine(); ImGui::Text(" | ");
        ImGui::SameLine();
        ImGui::Text("Undo: %s", ctx.GetCommandSystem().GetUndoCommandName().c_str());
    }
    
    ImGui::End();
}

}  // namespace mst
