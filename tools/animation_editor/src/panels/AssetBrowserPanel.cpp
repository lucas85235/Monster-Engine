#include "AssetBrowserPanel.h"
#include "../core/EditorContext.h"

#include <imgui.h>
#include <filesystem>

namespace fs = std::filesystem;

AssetBrowserPanel::AssetBrowserPanel(EditorContext& context) : context_(context) {
    RefreshAssets();
}

AssetBrowserPanel::~AssetBrowserPanel() = default;

void AssetBrowserPanel::Render() {
    ImGui::Begin("Asset Browser");

    if (ImGui::Button("Refresh")) {
        RefreshAssets();
    }

    ImGui::Separator();
    ImGui::Text("Animation Clips (%zu)", animationClips_.size());

    ImGui::BeginChild("ClipsList", ImVec2(0, 0), true);

    for (size_t i = 0; i < animationClips_.size(); ++i) {
        ImGui::PushID(static_cast<int>(i));
        
        bool isSelected = (selectedClip_ == static_cast<int>(i));
        
        if (ImGui::Selectable(animationClips_[i].filename.c_str(), isSelected)) {
            selectedClip_ = static_cast<int>(i);
            context_.SetSelectedClip(animationClips_[i].fullPath);
        }
        
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            const char* path = animationClips_[i].fullPath.c_str();
            ImGui::SetDragDropPayload("ANIMATION_CLIP", path, strlen(path) + 1);
            ImGui::Text("Clip: %s", animationClips_[i].filename.c_str());
            ImGui::EndDragDropSource();
        }
        
        if (ImGui::IsItemHovered()) {
            ImGui::BeginTooltip();
            ImGui::Text("Path: %s", animationClips_[i].fullPath.c_str());
            ImGui::EndTooltip();
        }
        
        ImGui::PopID();
    }

    ImGui::EndChild();
    ImGui::End();
}

void AssetBrowserPanel::RefreshAssets() {
    animationClips_.clear();

    std::vector<std::string> searchPaths = {"assets", "."};
    
    for (const auto& searchPath : searchPaths) {
        if (!fs::exists(searchPath)) continue;
        
        try {
            for (const auto& entry : fs::recursive_directory_iterator(searchPath)) {
                if (!entry.is_regular_file()) continue;
                
                std::string ext = entry.path().extension().string();
                std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
                
                if (ext == ".fbx" || ext == ".gltf" || ext == ".glb" || ext == ".dae") {
                    std::string filename = entry.path().filename().string();
                    std::string fullPath = entry.path().string();
                    
                    std::replace(fullPath.begin(), fullPath.end(), '\\', '/');
                    
                    animationClips_.push_back({filename, fullPath});
                }
            }
        } catch (const std::exception& e) {
        }
    }
    
    std::sort(animationClips_.begin(), animationClips_.end(), 
        [](const ClipInfo& a, const ClipInfo& b) { return a.filename < b.filename; });
}

const std::string& AssetBrowserPanel::GetSelectedClipPath() const {
    static std::string empty;
    if (selectedClip_ >= 0 && selectedClip_ < static_cast<int>(animationClips_.size())) {
        return animationClips_[selectedClip_].fullPath;
    }
    return empty;
}
