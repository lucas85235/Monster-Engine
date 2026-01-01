#include "FileDialogManager.h"

#include <imgui.h>
#include <algorithm>

#include "../core/EditorContext.h"
#include "engine/Log.h"

namespace mst {

FileDialogManager::FileDialogManager() {
    currentPath_ = std::filesystem::current_path();
}

void FileDialogManager::ShowSaveDialog(DialogCallback callback) {
    showSave_ = true;
    saveCallback_ = std::move(callback);
    selectedIndex_ = -1;
    RefreshDirectory();
}

void FileDialogManager::ShowOpenDialog(DialogCallback callback) {
    showOpen_ = true;
    openCallback_ = std::move(callback);
    selectedIndex_ = -1;
    RefreshDirectory();
}

void FileDialogManager::ShowOpenTextureDialog(DialogCallback callback) {
    showOpenTexture_ = true;
    textureCallback_ = std::move(callback);
    selectedIndex_ = -1;
    RefreshDirectory();
}

void FileDialogManager::Render(EditorContext& ctx) {
    if (showSave_) RenderSaveDialog();
    if (showOpen_) RenderOpenDialog();
    if (showOpenTexture_) RenderOpenTextureDialog();
}

void FileDialogManager::RefreshDirectory() {
    entries_.clear();
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(currentPath_)) {
            FileEntry fe;
            fe.name = entry.path().filename().string();
            fe.isDirectory = entry.is_directory();
            fe.size = fe.isDirectory ? 0 : entry.file_size();
            
            // Apply appropriate filter based on dialog type
            if (!fe.isDirectory) {
                if (showOpen_ && entry.path().extension() != fileExtension_) {
                    continue;
                }
                if (showOpenTexture_ && !MatchesFilter(fe.name)) {
                    continue;
                }
            }
            
            entries_.push_back(fe);
        }
        
        // Sort: directories first, then files
        std::sort(entries_.begin(), entries_.end(), [](const FileEntry& a, const FileEntry& b) {
            if (a.isDirectory != b.isDirectory) return a.isDirectory > b.isDirectory;
            return a.name < b.name;
        });
    } catch (const std::exception& e) {
        SE_LOG_ERROR("FileDialogManager: Failed to read directory: {}", e.what());
    }
}

bool FileDialogManager::MatchesFilter(const std::string& filename) const {
    std::filesystem::path p(filename);
    std::string ext = p.extension().string();
    // Convert to lowercase for comparison
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    for (const auto& texExt : textureExtensions_) {
        if (ext == texExt) return true;
    }
    return false;
}

void FileDialogManager::NavigateTo(const std::filesystem::path& path) {
    if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
        currentPath_ = path;
        selectedIndex_ = -1;
        RefreshDirectory();
    }
}

void FileDialogManager::RenderSaveDialog() {
    ImGui::OpenPopup("Save Map");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 450), ImGuiCond_FirstUseEver);
    
    if (ImGui::BeginPopupModal("Save Map", &showSave_)) {
        RenderFileBrowser(true);
        ImGui::EndPopup();
    }
    
    if (!showSave_ && saveCallback_) {
        saveCallback_(FileDialogResult{});
        saveCallback_ = nullptr;
    }
}

void FileDialogManager::RenderOpenDialog() {
    ImGui::OpenPopup("Open Map");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 450), ImGuiCond_FirstUseEver);
    
    if (ImGui::BeginPopupModal("Open Map", &showOpen_)) {
        RenderFileBrowser(false);
        ImGui::EndPopup();
    }
    
    if (!showOpen_ && openCallback_) {
        openCallback_(FileDialogResult{});
        openCallback_ = nullptr;
    }
}

void FileDialogManager::RenderOpenTextureDialog() {
    ImGui::OpenPopup("Select Texture");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(600, 450), ImGuiCond_FirstUseEver);
    
    if (ImGui::BeginPopupModal("Select Texture", &showOpenTexture_)) {
        // Current path display and navigation
        ImGui::Text("Location:");
        ImGui::SameLine();
        std::string pathStr = currentPath_.string();
        ImGui::TextWrapped("%s", pathStr.c_str());
        
        ImGui::SameLine(ImGui::GetWindowWidth() - 80);
        if (ImGui::Button("Up")) {
            if (currentPath_.has_parent_path() && currentPath_.parent_path() != currentPath_) {
                NavigateTo(currentPath_.parent_path());
            }
        }
        
        // Filter info
        ImGui::TextDisabled("Showing: PNG, JPG, TGA, BMP, HDR, PSD");
        ImGui::Separator();
        
        // File list
        ImVec2 listSize(0, -50);
        if (ImGui::BeginChild("TextureList", listSize, true)) {
            for (int i = 0; i < static_cast<int>(entries_.size()); ++i) {
                const auto& entry = entries_[i];
                
                const char* icon = entry.isDirectory ? "[DIR] " : "      ";
                std::string label = icon + entry.name;
                
                bool isSelected = (selectedIndex_ == i);
                if (ImGui::Selectable(label.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick)) {
                    selectedIndex_ = i;
                    
                    if (ImGui::IsMouseDoubleClicked(0)) {
                        if (entry.isDirectory) {
                            NavigateTo(currentPath_ / entry.name);
                        } else {
                            // Double-click on texture = select it
                            FileDialogResult result;
                            result.confirmed = true;
                            result.filename = entry.name;
                            result.fullPath = (currentPath_ / entry.name).string();
                            
                            if (textureCallback_) {
                                textureCallback_(result);
                                textureCallback_ = nullptr;
                            }
                            showOpenTexture_ = false;
                            ImGui::CloseCurrentPopup();
                        }
                    }
                }
            }
        }
        ImGui::EndChild();
        
        ImGui::Separator();
        
        // Selected file display
        if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(entries_.size()) && !entries_[selectedIndex_].isDirectory) {
            ImGui::Text("Selected: %s", entries_[selectedIndex_].name.c_str());
        } else {
            ImGui::TextDisabled("No texture selected");
        }
        
        // Action buttons
        bool canConfirm = selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(entries_.size()) && !entries_[selectedIndex_].isDirectory;
        
        if (!canConfirm) ImGui::BeginDisabled();
        if (ImGui::Button("Select", ImVec2(120, 0))) {
            FileDialogResult result;
            result.confirmed = true;
            result.filename = entries_[selectedIndex_].name;
            result.fullPath = (currentPath_ / result.filename).string();
            
            if (textureCallback_) {
                textureCallback_(result);
                textureCallback_ = nullptr;
            }
            showOpenTexture_ = false;
            ImGui::CloseCurrentPopup();
        }
        if (!canConfirm) ImGui::EndDisabled();
        
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(120, 0))) {
            showOpenTexture_ = false;
            ImGui::CloseCurrentPopup();
        }
        
        ImGui::EndPopup();
    }
    
    if (!showOpenTexture_ && textureCallback_) {
        textureCallback_(FileDialogResult{});
        textureCallback_ = nullptr;
    }
}

void FileDialogManager::RenderFileBrowser(bool isSaveMode) {
    // Current path display and navigation
    ImGui::Text("Location:");
    ImGui::SameLine();
    
    std::string pathStr = currentPath_.string();
    ImGui::TextWrapped("%s", pathStr.c_str());
    
    // Parent directory button
    ImGui::SameLine(ImGui::GetWindowWidth() - 80);
    if (ImGui::Button("Up")) {
        if (currentPath_.has_parent_path() && currentPath_.parent_path() != currentPath_) {
            NavigateTo(currentPath_.parent_path());
        }
    }
    
    ImGui::Separator();
    
    // File list
    ImVec2 listSize(0, -70);
    if (ImGui::BeginChild("FileList", listSize, true)) {
        for (int i = 0; i < static_cast<int>(entries_.size()); ++i) {
            const auto& entry = entries_[i];
            
            // Icon prefix
            const char* icon = entry.isDirectory ? "[DIR] " : "      ";
            std::string label = icon + entry.name;
            
            bool isSelected = (selectedIndex_ == i);
            if (ImGui::Selectable(label.c_str(), isSelected, ImGuiSelectableFlags_AllowDoubleClick)) {
                selectedIndex_ = i;
                
                if (!entry.isDirectory) {
                    // Copy filename to input (without extension)
                    auto stem = std::filesystem::path(entry.name).stem().string();
                    strncpy(fileName_, stem.c_str(), sizeof(fileName_) - 1);
                    fileName_[sizeof(fileName_) - 1] = '\0';
                }
                
                if (ImGui::IsMouseDoubleClicked(0)) {
                    if (entry.isDirectory) {
                        NavigateTo(currentPath_ / entry.name);
                    } else if (!isSaveMode) {
                        // Double-click on file in open mode = open file
                        FileDialogResult result;
                        result.confirmed = true;
                        result.filename = entry.name;
                        result.fullPath = (currentPath_ / entry.name).string();
                        
                        if (openCallback_) {
                            openCallback_(result);
                            openCallback_ = nullptr;
                        }
                        showOpen_ = false;
                        ImGui::CloseCurrentPopup();
                    }
                }
            }
        }
    }
    ImGui::EndChild();
    
    ImGui::Separator();
    
    // Filename input (for save mode)
    if (isSaveMode) {
        ImGui::Text("File name:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(300);
        ImGui::InputText("##filename", fileName_, sizeof(fileName_));
        ImGui::SameLine();
        ImGui::TextDisabled("%s", fileExtension_.c_str());
    } else {
        // For open mode, show selected file
        if (selectedIndex_ >= 0 && selectedIndex_ < static_cast<int>(entries_.size())) {
            ImGui::Text("Selected: %s", entries_[selectedIndex_].name.c_str());
        } else {
            ImGui::TextDisabled("No file selected");
        }
    }
    
    // Action buttons
    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 30);
    
    const char* actionLabel = isSaveMode ? "Save" : "Open";
    bool canConfirm = isSaveMode ? (strlen(fileName_) > 0) : 
                      (selectedIndex_ >= 0 && !entries_[selectedIndex_].isDirectory);
    
    if (!canConfirm) ImGui::BeginDisabled();
    
    if (ImGui::Button(actionLabel, ImVec2(120, 0))) {
        FileDialogResult result;
        result.confirmed = true;
        
        if (isSaveMode) {
            result.filename = std::string(fileName_) + fileExtension_;
            result.fullPath = (currentPath_ / result.filename).string();
            
            if (saveCallback_) {
                saveCallback_(result);
                saveCallback_ = nullptr;
            }
            showSave_ = false;
        } else {
            result.filename = entries_[selectedIndex_].name;
            result.fullPath = (currentPath_ / result.filename).string();
            
            if (openCallback_) {
                openCallback_(result);
                openCallback_ = nullptr;
            }
            showOpen_ = false;
        }
        
        ImGui::CloseCurrentPopup();
    }
    
    if (!canConfirm) ImGui::EndDisabled();
    
    ImGui::SameLine();
    
    if (ImGui::Button("Cancel", ImVec2(120, 0))) {
        if (isSaveMode) {
            showSave_ = false;
        } else {
            showOpen_ = false;
        }
        ImGui::CloseCurrentPopup();
    }
}

}  // namespace mst
