#include "ui/PropertiesPanel.h"

#include <imgui.h>

#include "core/EditorContext.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/renderer/TextureMaterial.h"
#include "engine/resources/TextureManager.h"
#include "engine/Log.h"

namespace mst {

void PropertiesPanel::Render(SelectionManager& selection, GizmoController& gizmo, EditorContext& context) {
    ImGui::Begin("Properties");

    RenderGizmoControls(gizmo);

    ImGui::Separator();

    if (!selection.HasSelection()) {
        ImGui::TextDisabled("No entity selected");
        ImGui::End();
        return;
    }

    se::Entity entity = selection.GetPrimarySelection();
    if (!entity.IsValid()) {
        ImGui::TextDisabled("Invalid entity");
        ImGui::End();
        return;
    }

    // Entity name
    auto& nameComp = entity.GetComponent<se::NameComponent>();
    char  nameBuffer[256];
    strncpy(nameBuffer, nameComp.Name.c_str(), sizeof(nameBuffer) - 1);
    nameBuffer[sizeof(nameBuffer) - 1] = '\0';

    if (ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer))) {
        nameComp.Name = nameBuffer;
    }

    ImGui::Separator();

    // Transform
    if (entity.HasComponent<se::TransformComponent>()) {
        auto& transform = entity.GetComponent<se::TransformComponent>();
        RenderTransform(transform);
    }

    ImGui::Separator();

    // Editor metadata (collision)
    if (entity.HasComponent<PrimitiveFactory::EditorMetadata>()) {
        auto& metadata = entity.GetComponent<PrimitiveFactory::EditorMetadata>();
        RenderEditorMetadata(metadata);
        
        ImGui::Separator();
        
        // Material assignment - pass entity to apply to MeshRenderComponent
        RenderMaterial(metadata, context, entity);
    }

    ImGui::Separator();

    // Mesh renderer
    if (entity.HasComponent<se::MeshRenderComponent>()) {
        auto& mesh = entity.GetComponent<se::MeshRenderComponent>();
        RenderMeshRenderer(mesh);
    }

    ImGui::End();
}

void PropertiesPanel::RenderTransform(se::TransformComponent& transform) {
    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        Vector3 position = transform.Position;
        Vector3 rotation = transform.Rotation;
        Vector3 scale    = transform.Scale;

        if (ImGui::DragFloat3("Position", &position.x, 0.1f)) {
            transform.SetPosition(position);
        }

        if (ImGui::DragFloat3("Rotation", &rotation.x, 1.0f)) {
            transform.SetRotation(rotation);
        }

        if (ImGui::DragFloat3("Scale", &scale.x, 0.1f, 0.01f, 100.0f)) {
            transform.SetScale(scale);
        }
    }
}

void PropertiesPanel::RenderMeshRenderer(se::MeshRenderComponent& mesh) {
    if (ImGui::CollapsingHeader("Mesh Renderer", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Visible", &mesh.IsVisible);
        ImGui::Checkbox("Cast Shadows", &mesh.CastShadows);
        ImGui::Checkbox("Receive Shadows", &mesh.ReceiveShadows);
        ImGui::ColorEdit4("Color", &mesh.Color.x);
        
        ImGui::Separator();
        ImGui::Text("Emissive (GI)");
        ImGui::ColorEdit3("Emissive Color", &mesh.EmissiveColor.x);
        ImGui::DragFloat("Emissive Factor", &mesh.EmissiveFactor, 0.1f, 0.0f, 10.0f, "%.2f");
    }
}

void PropertiesPanel::RenderEditorMetadata(PrimitiveFactory::EditorMetadata& metadata) {
    if (ImGui::CollapsingHeader("Collision", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Primitive: %s", PrimitiveTypeToString(metadata.primitiveType));

        ImGui::Checkbox("Has Collision", &metadata.hasCollision);

        if (metadata.hasCollision) {
            const char* colliderTypes[] = {"None", "Box", "Sphere", "Capsule"};
            int         currentType     = static_cast<int>(metadata.colliderType);

            if (ImGui::Combo("Collider Type", &currentType, colliderTypes, 4)) {
                metadata.colliderType = static_cast<ColliderType>(currentType);
            }

            switch (metadata.colliderType) {
                case ColliderType::Box:
                    ImGui::DragFloat3("Size", &metadata.colliderSize.x, 0.1f, 0.01f, 100.0f);
                    break;

                case ColliderType::Sphere:
                    ImGui::DragFloat("Radius", &metadata.colliderRadius, 0.1f, 0.01f, 100.0f);
                    break;

                case ColliderType::Capsule:
                    ImGui::DragFloat("Radius", &metadata.colliderRadius, 0.1f, 0.01f, 100.0f);
                    ImGui::DragFloat("Height", &metadata.colliderHeight, 0.1f, 0.01f, 100.0f);
                    break;

                default: break;
            }
            
            ImGui::Separator();
            ImGui::Text("Rigidbody");
            
            const char* rigidbodyTypes[] = {"Static", "Dynamic", "Kinematic"};
            int currentRbType = static_cast<int>(metadata.rigidbodyType);
            if (ImGui::Combo("Body Type", &currentRbType, rigidbodyTypes, 3)) {
                metadata.rigidbodyType = static_cast<uint8_t>(currentRbType);
            }
            
            if (metadata.rigidbodyType == 1) {  // Dynamic
                ImGui::DragFloat("Mass", &metadata.mass, 0.1f, 0.01f, 1000.0f, "%.2f kg");
            }
        }
    }
}

void PropertiesPanel::RenderMaterial(PrimitiveFactory::EditorMetadata& metadata, EditorContext& context, se::Entity entity) {
    if (ImGui::CollapsingHeader("Material", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Use Custom Material", &metadata.hasCustomMaterial);
        
        if (metadata.hasCustomMaterial) {
            // Build combo items from material library
            const auto& materials = context.GetMaterials();
            
            if (materials.empty()) {
                ImGui::TextDisabled("No materials available");
                if (ImGui::Button("Create Material")) {
                    context.CreateNewMaterial();
                }
            } else {
                // Find current selection index
                int currentIndex = -1;
                for (size_t i = 0; i < materials.size(); ++i) {
                    if (materials[i].name == metadata.materialName) {
                        currentIndex = static_cast<int>(i);
                        break;
                    }
                }
                
                // Capture previous state BEFORE combo changes it
                int previousIndex = currentIndex;
                
                // Material dropdown
                if (ImGui::BeginCombo("##MaterialCombo", 
                    currentIndex >= 0 ? materials[currentIndex].name.c_str() : "(None)")) {
                    for (size_t i = 0; i < materials.size(); ++i) {
                        ImGui::PushID(static_cast<int>(i));
                        bool isSelected = (currentIndex == static_cast<int>(i));
                        if (ImGui::Selectable(materials[i].name.c_str(), isSelected)) {
                            metadata.materialName = materials[i].name;
                            currentIndex = static_cast<int>(i);
                        }
                        if (isSelected) {
                            ImGui::SetItemDefaultFocus();
                        }
                        ImGui::PopID();
                    }
                    ImGui::EndCombo();
                }
                ImGui::SameLine();
                ImGui::TextDisabled("Material");
                
                // Detect if combo selection changed this frame
                bool selectionChanged = (currentIndex != previousIndex);
                
                // Also check if material is not yet applied to the mesh component
                bool needsInitialLoad = false;
                bool materialModified = false;
                if (currentIndex >= 0 && entity.HasComponent<se::MeshRenderComponent>()) {
                    auto& meshRender = entity.GetComponent<se::MeshRenderComponent>();
                    needsInitialLoad = (meshRender.customTextureMaterial == nullptr && metadata.hasCustomMaterial);
                    // Detect if material was modified in Material Editor
                    materialModified = materials[currentIndex].isDirty;
                }
                
                // Apply material to MeshRenderComponent on change, initial load, or when modified
                if (currentIndex >= 0 && entity.HasComponent<se::MeshRenderComponent>()) {
                    auto& mat = const_cast<EditorMaterialData&>(materials[currentIndex]);
                    auto& meshRender = entity.GetComponent<se::MeshRenderComponent>();
                    
                    if (selectionChanged || needsInitialLoad || materialModified) {
                        // Apply PBR params from material to MeshRenderComponent
                        meshRender.UseCustomPBR = true;
                        meshRender.Metallic = mat.metallic;
                        meshRender.Roughness = mat.roughness;
                        meshRender.Reflectance = mat.reflectance;
                        meshRender.AO = mat.ao;
                        meshRender.Color = mat.baseColor;
                        meshRender.EmissiveColor = mat.emissiveColor;
                        meshRender.EmissiveFactor = mat.emissiveFactor;
                        
                        // Create TextureMaterial using cached TextureManager (no per-frame loading)
                        auto texMat = std::make_shared<se::TextureMaterial>();
                        texMat->BaseColor = mat.baseColor;
                        texMat->MetallicFactor = mat.metallic;
                        texMat->RoughnessFactor = mat.roughness;
                        
                        // Load textures using TextureManager (cached - fast lookup)
                        if (mat.useAlbedoTexture && !mat.albedoTexturePath.empty()) {
                            texMat->Albedo = se::TextureManager::Load(mat.albedoTexturePath);
                        }
                        if (mat.useNormalTexture && !mat.normalTexturePath.empty()) {
                            texMat->Normal = se::TextureManager::Load(mat.normalTexturePath);
                        }
                        if (mat.useMetallicTexture && !mat.metallicTexturePath.empty()) {
                            texMat->Metallic = se::TextureManager::Load(mat.metallicTexturePath);
                        }
                        if (mat.useRoughnessTexture && !mat.roughnessTexturePath.empty()) {
                            texMat->Roughness = se::TextureManager::Load(mat.roughnessTexturePath);
                        }
                        if (mat.useAOTexture && !mat.aoTexturePath.empty()) {
                            texMat->AO = se::TextureManager::Load(mat.aoTexturePath);
                        }
                        if (mat.useEmissiveTexture && !mat.emissiveTexturePath.empty()) {
                            texMat->Emissive = se::TextureManager::Load(mat.emissiveTexturePath);
                        }
                        
                        meshRender.customTextureMaterial = texMat;
                        
                        // Clear dirty flag after applying to this entity
                        mat.isDirty = false;
                        
                        SE_LOG_INFO("PropertiesPanel: Applied material '{}' to entity (modified={})", 
                                    mat.name, materialModified);
                    }
                    
                    // Quick preview of selected material (UI only, no loading)
                    ImGui::TextDisabled("Metallic: %.2f  Roughness: %.2f", mat.metallic, mat.roughness);
                    if (mat.HasAnyTexture()) {
                        ImGui::TextDisabled("Textures: %s%s%s%s%s%s",
                            mat.useAlbedoTexture ? "A" : "",
                            mat.useNormalTexture ? "N" : "",
                            mat.useMetallicTexture ? "M" : "",
                            mat.useRoughnessTexture ? "R" : "",
                            mat.useAOTexture ? "O" : "",
                            mat.useEmissiveTexture ? "E" : "");
                    }
                }
            }
        } else {
            ImGui::TextDisabled("Using default material");
            
            // Reset custom PBR if disabled
            if (entity.HasComponent<se::MeshRenderComponent>()) {
                auto& meshRender = entity.GetComponent<se::MeshRenderComponent>();
                meshRender.UseCustomPBR = false;
            }
        }
    }
}

void PropertiesPanel::RenderGizmoControls(GizmoController& gizmo) {
    if (ImGui::CollapsingHeader("Gizmo", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Text("Operation: %s", gizmo.GetOperationName());
        ImGui::Text("Space: %s", gizmo.GetSpaceName());

        ImGui::Separator();

        if (ImGui::Button("Translate (W)")) {
            gizmo.SetOperation(GizmoController::Operation::Translate);
        }
        ImGui::SameLine();
        if (ImGui::Button("Rotate (E)")) {
            gizmo.SetOperation(GizmoController::Operation::Rotate);
        }
        ImGui::SameLine();
        if (ImGui::Button("Scale (R)")) {
            gizmo.SetOperation(GizmoController::Operation::Scale);
        }

        if (ImGui::Button("Toggle Space (Q)")) {
            gizmo.ToggleSpace();
        }
    }
}

}  // namespace mst

