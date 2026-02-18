#include "ui/MaterialEditorPanel.h"

#include <imgui.h>

#include "core/EditorContext.h"
#include "core/MaterialSerializer.h"
#include "ui/FileDialogManager.h"
#include "engine/Log.h"
// TODO: Replace with Filament-compatible texture system
// #include "engine/renderer/Texture.h"
// #include "engine/resources/TextureManager.h"

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
                RenderTextureSlots(*material, context);

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
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("The diffuse color of the material.\nFor metals, this defines the specular color.");
        }
        
        // Core parameters
        changed |= ImGui::SliderFloat("Metallic", &material.metallic, 0.0f, 1.0f, "%.2f");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("0.0 = Dielectric (plastic, wood, skin)\n1.0 = Metal (gold, silver, copper)");
        }
        
        changed |= ImGui::SliderFloat("Roughness", &material.roughness, 0.0f, 1.0f, "%.2f");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("0.0 = Mirror-like reflection\n1.0 = Completely matte/diffuse");
        }
        
        changed |= ImGui::SliderFloat("Reflectance", &material.reflectance, 0.0f, 1.0f, "%.2f");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Fresnel reflectance at normal incidence.\n0.5 = 4%% (common dielectrics like plastic)\n0.35 = Water\n1.0 = Crystal");
        }
        
        changed |= ImGui::SliderFloat("AO", &material.ao, 0.0f, 1.0f, "%.2f");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Ambient Occlusion multiplier.\n1.0 = No occlusion\n0.0 = Fully occluded (darker crevices)");
        }
        
        changed |= ImGui::SliderFloat("Normal Scale", &material.normalScale, 0.0f, 2.0f, "%.2f");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Scales the effect of normal maps.\n1.0 = Default strength\n0.0 = Disabled");
        }

        ImGui::Separator();
        ImGui::Text("Emissive");
        changed |= ImGui::ColorEdit3("Emissive Color", &material.emissiveColor.x);
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("The color of light emitted by this material.\nUsed for GI (Global Illumination).");
        }
        
        changed |= ImGui::DragFloat("Emissive Factor", &material.emissiveFactor, 0.1f, 0.0f, 10.0f, "%.2f");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Intensity of emitted light.\n0.0 = No emission\n>1.0 = Bright light source");
        }

        if (changed) {
            material.isDirty = true;
        }
    }
}

void MaterialEditorPanel::RenderTextureSlots(EditorMaterialData& material, EditorContext& context) {
    if (ImGui::CollapsingHeader("Textures", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool changed = false;
        
        auto RenderTextureSlot = [&](const char* label, std::string& path, bool& useTexture) {
            ImGui::PushID(label);
            
            changed |= ImGui::Checkbox("##use", &useTexture);
            ImGui::SameLine();
            
            // Display texture thumbnail using ImGui::Image if texture exists
            bool clicked = false;
            if (!path.empty() && useTexture) {
                // TODO: Implement Filament-compatible texture preview
                // Legacy OpenGL texture preview disabled
                if (ImGui::Button("[tex]##thumb", ImVec2(32, 32))) {
                    clicked = true;
                }
            } else {
                // No texture - show empty placeholder
                if (ImGui::Button("##thumb", ImVec2(32, 32))) {
                    clicked = true;
                }
            }
            
            // Open file dialog when thumbnail is clicked
            if (clicked) {
                pendingTexturePath_ = &path;
                pendingTextureUse_ = &useTexture;
                if (auto* fdm = context.GetFileDialogs()) {
                    fdm->ShowOpenTextureDialog([this, &material](const FileDialogResult& result) {
                        if (result.confirmed && pendingTexturePath_ && pendingTextureUse_) {
                            *pendingTexturePath_ = result.fullPath;
                            *pendingTextureUse_ = true;
                            material.isDirty = true;
                            SE_LOG_INFO("MaterialEditor: Loaded texture: {}", result.fullPath);
                        }
                        pendingTexturePath_ = nullptr;
                        pendingTextureUse_ = nullptr;
                    });
                }
            }
            
            if (ImGui::IsItemHovered() && !path.empty()) {
                ImGui::SetTooltip("%s", path.c_str());
            } else if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Click to select texture");
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
                pendingTexturePath_ = &path;
                pendingTextureUse_ = &useTexture;
                if (auto* fdm = context.GetFileDialogs()) {
                    fdm->ShowOpenTextureDialog([this, &material](const FileDialogResult& result) {
                        if (result.confirmed && pendingTexturePath_ && pendingTextureUse_) {
                            *pendingTexturePath_ = result.fullPath;
                            *pendingTextureUse_ = true;
                            material.isDirty = true;
                            SE_LOG_INFO("MaterialEditor: Loaded texture: {}", result.fullPath);
                        }
                        pendingTexturePath_ = nullptr;
                        pendingTextureUse_ = nullptr;
                    });
                }
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
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Clear Coat");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Adds a transparent glossy layer on top.\nUsed for car paint, lacquered wood, etc.");
        }
        changed |= ImGui::SliderFloat("Intensity##cc", &material.clearCoat, 0.0f, 1.0f, "%.2f");
        changed |= ImGui::SliderFloat("Roughness##cc", &material.clearCoatRoughness, 0.0f, 1.0f, "%.2f");

        ImGui::Separator();

        // Anisotropy
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Anisotropy");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Stretches reflections in one direction.\nUsed for brushed metal, hair, vinyl records.");
        }
        changed |= ImGui::SliderFloat("Amount", &material.anisotropy, -1.0f, 1.0f, "%.2f");
        changed |= ImGui::DragFloat3("Direction", &material.anisotropyDirection.x, 0.01f, -1.0f, 1.0f);

        ImGui::Separator();

        // Sheen
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Sheen (Fabric)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Adds soft retroreflection at grazing angles.\nUsed for fabric, velvet, cloth materials.");
        }
        changed |= ImGui::ColorEdit3("Sheen Color", &material.sheenColor.x);
        changed |= ImGui::SliderFloat("Sheen Roughness", &material.sheenRoughness, 0.0f, 1.0f, "%.2f");

        ImGui::Separator();

        // Subsurface
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Subsurface");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Light scattering inside the material.\nUsed for skin, wax, marble, jade.");
        }
        changed |= ImGui::ColorEdit3("Sub Color", &material.subsurfaceColor.x);
        changed |= ImGui::SliderFloat("Power", &material.subsurfacePower, 0.0f, 10.0f, "%.2f");
        changed |= ImGui::SliderFloat("Thickness", &material.thickness, 0.0f, 1.0f, "%.2f");

        ImGui::Separator();

        // Transmission
        ImGui::TextColored(ImVec4(0.6f, 0.8f, 1.0f, 1.0f), "Transmission (Glass)");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Light passing through the material.\nUsed for glass, water, crystals.");
        }
        changed |= ImGui::SliderFloat("Transmission", &material.transmission, 0.0f, 1.0f, "%.2f");
        changed |= ImGui::DragFloat("IOR", &material.ior, 0.01f, 1.0f, 3.0f, "%.3f");
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Index of Refraction.\n1.0 = Air\n1.33 = Water\n1.45 = Plastic\n1.52 = Glass\n2.42 = Diamond");
        }

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
    
    // Primary save button
    if (ImGui::Button("Save")) {
        if (!material->filePath.empty()) {
            // Save to existing path
            if (MaterialSerializer::Save(*material, material->filePath)) {
                material->isDirty = false;
                SE_LOG_INFO("MaterialEditor: Saved material to '{}'", material->filePath);
            } else {
                SE_LOG_ERROR("MaterialEditor: Failed to save material to '{}'", material->filePath);
            }
        } else {
            // Open save dialog for new material
            if (auto* fdm = context.GetFileDialogs()) {
                fdm->ShowSaveDialog([this, material](const FileDialogResult& result) {
                    if (result.confirmed && !result.fullPath.empty()) {
                        std::string path = result.fullPath;
                        // Ensure .mstmat extension
                        if (path.find(".mstmat") == std::string::npos) {
                            path += ".mstmat";
                        }
                        if (MaterialSerializer::Save(*material, path)) {
                            material->filePath = path;
                            material->isDirty = false;
                            SE_LOG_INFO("MaterialEditor: Saved material to '{}'", path);
                        } else {
                            SE_LOG_ERROR("MaterialEditor: Failed to save material to '{}'", path);
                        }
                    }
                });
            }
        }
    }
    if (ImGui::IsItemHovered()) {
        if (material->filePath.empty()) {
            ImGui::SetTooltip("Save material binary (will prompt for location)");
        } else {
            ImGui::SetTooltip("Save to: %s", material->filePath.c_str());
        }
    }
    ImGui::SameLine();
    
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
