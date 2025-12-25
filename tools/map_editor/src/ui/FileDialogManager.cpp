#include "FileDialogManager.h"

#include <imgui.h>

#include "../core/EditorContext.h"

namespace mst {

void FileDialogManager::ShowExportDialog(DialogCallback callback) {
    showExport_ = true;
    exportCallback_ = std::move(callback);
}

void FileDialogManager::ShowOpenDialog(DialogCallback callback) {
    showOpen_ = true;
    openCallback_ = std::move(callback);
}

void FileDialogManager::Render(EditorContext& ctx) {
    if (showExport_) RenderExportDialog();
    if (showOpen_) RenderOpenDialog();
}

void FileDialogManager::RenderExportDialog() {
    ImGui::OpenPopup("Export Map");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    
    if (ImGui::BeginPopupModal("Export Map", &showExport_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Export map as .mstmap file");
        ImGui::Separator();
        
        ImGui::InputText("File Name", exportFileName_, sizeof(exportFileName_));
        
        ImGui::Separator();
        
        if (ImGui::Button("Export", ImVec2(120, 0))) {
            FileDialogResult result;
            result.confirmed = true;
            result.filename = std::string(exportFileName_) + ".mstmap";
            
            if (exportCallback_) {
                exportCallback_(result);
            }
            
            showExport_ = false;
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            if (exportCallback_) {
                exportCallback_(FileDialogResult{});
            }
            showExport_ = false;
        }
        
        ImGui::EndPopup();
    }
}

void FileDialogManager::RenderOpenDialog() {
    ImGui::OpenPopup("Open Map");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    
    if (ImGui::BeginPopupModal("Open Map", &showOpen_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Open a .mstmap file");
        ImGui::Separator();
        
        ImGui::InputText("File Name", openFileName_, sizeof(openFileName_));
        ImGui::Text("(Enter filename without extension, file must be in current directory)");
        
        ImGui::Separator();
        
        if (ImGui::Button("Open", ImVec2(120, 0))) {
            FileDialogResult result;
            result.confirmed = true;
            result.filename = std::string(openFileName_) + ".mstmap";
            
            if (openCallback_) {
                openCallback_(result);
            }
            
            showOpen_ = false;
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            if (openCallback_) {
                openCallback_(FileDialogResult{});
            }
            showOpen_ = false;
        }
        
        ImGui::EndPopup();
    }
}

}  // namespace mst
