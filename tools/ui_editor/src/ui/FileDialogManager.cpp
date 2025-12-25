#include "FileDialogManager.h"

#include <imgui.h>

namespace ued {

void FileDialogManager::ShowSaveDialog(DialogCallback callback) {
    showSave_ = true;
    saveCallback_ = std::move(callback);
}

void FileDialogManager::ShowOpenDialog(DialogCallback callback) {
    showOpen_ = true;
    openCallback_ = std::move(callback);
}

void FileDialogManager::Render() {
    if (showSave_) RenderSaveDialog();
    if (showOpen_) RenderOpenDialog();
}

void FileDialogManager::RenderSaveDialog() {
    ImGui::OpenPopup("Save UI Document");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    
    if (ImGui::BeginPopupModal("Save UI Document", &showSave_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Save UI document as .rml file");
        ImGui::Separator();
        
        ImGui::InputText("File Name", saveFileName_, sizeof(saveFileName_));
        
        ImGui::Separator();
        
        if (ImGui::Button("Save", ImVec2(120, 0))) {
            FileDialogResult result;
            result.confirmed = true;
            result.filename = std::string(saveFileName_) + ".rml";
            
            if (saveCallback_) {
                saveCallback_(result);
            }
            
            showSave_ = false;
        }
        
        ImGui::SameLine();
        
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            if (saveCallback_) {
                saveCallback_(FileDialogResult{});
            }
            showSave_ = false;
        }
        
        ImGui::EndPopup();
    }
}

void FileDialogManager::RenderOpenDialog() {
    ImGui::OpenPopup("Open UI Document");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    
    if (ImGui::BeginPopupModal("Open UI Document", &showOpen_, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text("Open a .rml file");
        ImGui::Separator();
        
        ImGui::InputText("File Name", openFileName_, sizeof(openFileName_));
        ImGui::TextDisabled("(Enter filename without extension)");
        
        ImGui::Separator();
        
        if (ImGui::Button("Open", ImVec2(120, 0))) {
            FileDialogResult result;
            result.confirmed = true;
            result.filename = std::string(openFileName_) + ".rml";
            
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

}  // namespace ued
