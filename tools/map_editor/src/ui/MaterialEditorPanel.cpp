#include "ui/MaterialEditorPanel.h"

#include <imgui.h>

#include "core/EditorContext.h"
#include "core/MaterialSerializer.h"
#include "engine/Log.h"

namespace mst {

void MaterialEditorPanel::Render(EditorContext& context) {
    if (!visible_) return;

    ImGui::SetNextWindowSize(ImVec2(500, 700), ImGuiCond_FirstUseEver);
    
    if (ImGui::Begin("Material Editor", &visible_, ImGuiWindowFlags_MenuBar)) {
        // Menu bar
        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New Material", "Ctrl+N")) {
                    context.CreateNewMaterial();
                    selectedMaterialIndex_ = static_cast<int>(context.GetMaterialCount()) - 1;
                }
                if (ImGui::MenuItem("Open Material...", "Ctrl+O")) {
                    // TODO: Open file dialog
                }
                ImGui::Separator();
                if (ImGui::MenuItem("Save", "Ctrl+S", false, selectedMaterialIndex_ >= 0)) {
                    if (selectedMaterialIndex_ >= 0) {
                        auto* mat = context.GetMaterialByIndex(selectedMaterialIndex_);
                        if (mat && !mat->filePath.empty()) {
                            MaterialSerializer::Save(*mat, mat->filePath);
                            mat->isDirty = false;
                        }
                    }
                }
                if (ImGui::MenuItem("Save As...", nullptr, false, selectedMaterialIndex_ >= 0)) {
                    // TODO: Open save file dialog
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Two-column layout: library on left, editor on right
        ImGui::Columns(2, "MaterialEditorColumns", true);
        ImGui::SetColumnWidth(0, 180);

        // Left column: Material Library
        RenderMaterialLibrary(context);

        ImGui::NextColumn();

        // Right column: Material Editor
        if (selectedMaterialIndex_ >= 0) {
            auto* material = context.GetMaterialByIndex(selectedMaterialIndex_);
            if (material) {
                // Material name
                char nameBuffer[256];
                strncpy(nameBuffer, material->name.c_str(), sizeof(nameBuffer) - 1);
                nameBuffer[sizeof(nameBuffer) - 1] = '\0';
                
                if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
                    material->name = nameBuffer;
                    material->isDirty = true;
                }
                
                ImGui::Separator();

                // PBR Parameters
                RenderPBRParameters(*material);

                ImGui::Separator();

                // Texture slots
                RenderTextureSlots(*material);

                ImGui::Separator();

                // Advanced parameters (collapsible)
                RenderAdvancedParameters(*material);

                ImGui::Separator();

                // Actions
                RenderActions(context);
            }
        } else {
            ImGui::TextDisabled("Select or create a material");
        }

        ImGui::Columns(1);
    }
    ImGui::End();
}

void MaterialEditorPanel::RenderMaterialLibrary(EditorContext& context) {
    ImGui::Text("Materials");
    ImGui::Separator();

    // Search filter
    ImGui::SetNextItemWidth(-1);
    ImGui::InputTextWithHint("##Search", "Search...", searchBuffer_, sizeof(searchBuffer_));

    // New material button
    if (ImGui::Button("+ New", ImVec2(-1, 0))) {
        context.CreateNewMaterial();
        selectedMaterialIndex_ = static_cast<int>(context.GetMaterialCount()) - 1;
    }

    ImGui::Separator();

    // Material list
    ImGui::BeginChild("MaterialList", ImVec2(0, 0), true);
    
    std::string searchStr(searchBuffer_);
    for (int i = 0; i < static_cast<int>(context.GetMaterialCount()); ++i) {
        auto* material = context.GetMaterialByIndex(i);
        if (!material) continue;

        // Filter by search
        if (!searchStr.empty()) {
            if (material->name.find(searchStr) == std::string::npos) {
                continue;
            }
        }

        // Display material entry
        std::string label = material->name;
        if (material->isDirty) {
            label += " *";
        }

        bool isSelected = (selectedMaterialIndex_ == i);
        if (ImGui::Selectable(label.c_str(), isSelected)) {
            selectedMaterialIndex_ = i;
        }

        // Context menu
        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Duplicate")) {
                context.DuplicateMaterial(i);
            }
            if (ImGui::MenuItem("Delete")) {
                context.RemoveMaterial(i);
                if (selectedMaterialIndex_ >= static_cast<int>(context.GetMaterialCount())) {
                    selectedMaterialIndex_ = static_cast<int>(context.GetMaterialCount()) - 1;
                }
            }
            ImGui::EndPopup();
        }
    }

    ImGui::EndChild();
}

void MaterialEditorPanel::RenderPBRParameters(EditorMaterialData& material) {
    if (ImGui::CollapsingHeader("PBR Parameters", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool changed = false;

        // Base Color
        changed |= ImGui::ColorEdit4("Base Color", &material.baseColor.x);
        
        // Core parameters
        changed |= ImGui::SliderFloat("Metallic", &material.metallic, 0.0f, 1.0f, "%.2f");
        changed |= ImGui::SliderFloat("Roughness", &material.roughness, 0.0f, 1.0f, "%.2f");
        changed |= ImGui::SliderFloat("Reflectance", &material.reflectance, 0.0f, 1.0f, "%.2f");
        
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Fresnel reflectance at normal incidence\n0.5 = 4%% (common dielectrics)");
        }
        
        changed |= ImGui::SliderFloat("AO", &material.ao, 0.0f, 1.0f, "%.2f");
        changed |= ImGui::SliderFloat("Normal Scale", &material.normalScale, 0.0f, 2.0f, "%.2f");

        ImGui::Separator();
        ImGui::Text("Emissive");
        changed |= ImGui::ColorEdit3("Emissive Color", &material.emissiveColor.x);
        changed |= ImGui::DragFloat("Emissive Factor", &material.emissiveFactor, 0.1f, 0.0f, 10.0f, "%.2f");

        if (changed) {
            material.isDirty = true;
        }
    }
}

void MaterialEditorPanel::RenderTextureSlots(EditorMaterialData& material) {
    if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool changed = false;
        
        auto RenderTextureSlot = [&](const char* label, std::string& path, bool& useTexture) {
            ImGui::PushID(label);
            
            changed |= ImGui::Checkbox("##use", &useTexture);
            ImGui::SameLine();
            
            // Thumbnail placeholder
            ImGui::Button("##thumb", ImVec2(32, 32));
            if (ImGui::IsItemHovered() && !path.empty()) {
                ImGui::SetTooltip("%s", path.c_str());
            }
            ImGui::SameLine();
            
            ImGui::BeginGroup();
            ImGui::Text("%s", label);
            
            // Path display (truncated)
            std::string displayPath = path.empty() ? "(none)" : path;
            if (displayPath.length() > 25) {
                displayPath = "..." + displayPath.substr(displayPath.length() - 22);
            }
            ImGui::TextDisabled("%s", displayPath.c_str());
            ImGui::EndGroup();
            
            ImGui::SameLine(ImGui::GetContentRegionAvail().x - 60);
            if (ImGui::Button("Load")) {
                // TODO: Open texture file dialog
                SE_LOG_INFO("MaterialEditor: Load texture for {}", label);
            }
            ImGui::SameLine();
            if (ImGui::Button("X") && !path.empty()) {
                path.clear();
                useTexture = false;
                changed = true;
            }
            
            ImGui::PopID();
        };
        
        RenderTextureSlot("Albedo", material.albedoTexturePath, material.useAlbedoTexture);
        RenderTextureSlot("Normal", material.normalTexturePath, material.useNormalTexture);
        RenderTextureSlot("Metallic", material.metallicTexturePath, material.useMetallicTexture);
        RenderTextureSlot("Roughness", material.roughnessTexturePath, material.useRoughnessTexture);
        RenderTextureSlot("AO", material.aoTexturePath, material.useAOTexture);
        RenderTextureSlot("Emissive", material.emissiveTexturePath, material.useEmissiveTexture);

        if (changed) {
            material.isDirty = true;
        }
    }
}

void MaterialEditorPanel::RenderAdvancedParameters(EditorMaterialData& material) {
    if (ImGui::CollapsingHeader("Advanced Parameters")) {
        bool changed = false;

        // Clear Coat
        ImGui::Text("Clear Coat");
        changed |= ImGui::SliderFloat("Intensity##cc", &material.clearCoat, 0.0f, 1.0f, "%.2f");
        changed |= ImGui::SliderFloat("Roughness##cc", &material.clearCoatRoughness, 0.0f, 1.0f, "%.2f");

        ImGui::Separator();

        // Anisotropy
        ImGui::Text("Anisotropy");
        changed |= ImGui::SliderFloat("Amount", &material.anisotropy, -1.0f, 1.0f, "%.2f");
        changed |= ImGui::DragFloat3("Direction", &material.anisotropyDirection.x, 0.01f, -1.0f, 1.0f);

        ImGui::Separator();

        // Sheen
        ImGui::Text("Sheen (Fabric)");
        changed |= ImGui::ColorEdit3("Sheen Color", &material.sheenColor.x);
        changed |= ImGui::SliderFloat("Sheen Roughness", &material.sheenRoughness, 0.0f, 1.0f, "%.2f");

        ImGui::Separator();

        // Subsurface
        ImGui::Text("Subsurface");
        changed |= ImGui::ColorEdit3("Sub Color", &material.subsurfaceColor.x);
        changed |= ImGui::SliderFloat("Power", &material.subsurfacePower, 0.0f, 10.0f, "%.2f");
        changed |= ImGui::SliderFloat("Thickness", &material.thickness, 0.0f, 1.0f, "%.2f");

        ImGui::Separator();

        // Transmission
        ImGui::Text("Transmission (Glass)");
        changed |= ImGui::SliderFloat("Transmission", &material.transmission, 0.0f, 1.0f, "%.2f");
        changed |= ImGui::DragFloat("IOR", &material.ior, 0.01f, 1.0f, 3.0f, "%.3f");

        if (changed) {
            material.isDirty = true;
        }
    }
}

void MaterialEditorPanel::RenderActions(EditorContext& context) {
    if (selectedMaterialIndex_ < 0) return;

    auto* material = context.GetMaterialByIndex(selectedMaterialIndex_);
    if (!material) return;

    ImGui::BeginGroup();
    
    if (ImGui::Button("Duplicate")) {
        context.DuplicateMaterial(selectedMaterialIndex_);
        selectedMaterialIndex_ = static_cast<int>(context.GetMaterialCount()) - 1;
    }
    ImGui::SameLine();
    
    if (ImGui::Button("Reset to Defaults")) {
        material->Reset();
    }
    ImGui::SameLine();
    
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.1f, 0.1f, 1.0f));
    if (ImGui::Button("Delete")) {
        context.RemoveMaterial(selectedMaterialIndex_);
        if (selectedMaterialIndex_ >= static_cast<int>(context.GetMaterialCount())) {
            selectedMaterialIndex_ = static_cast<int>(context.GetMaterialCount()) - 1;
        }
    }
    ImGui::PopStyleColor();

    ImGui::EndGroup();
}

}  // namespace mst
