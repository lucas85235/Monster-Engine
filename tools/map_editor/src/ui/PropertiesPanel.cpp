#include "ui/PropertiesPanel.h"

#include <imgui.h>

#include "engine/ecs/SimpleComponents.h"

namespace mst {

void PropertiesPanel::Render(SelectionManager& selection, GizmoController& gizmo) {
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

    // Editor metadata
    if (entity.HasComponent<PrimitiveFactory::EditorMetadata>()) {
        auto& metadata = entity.GetComponent<PrimitiveFactory::EditorMetadata>();
        RenderEditorMetadata(metadata);
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
