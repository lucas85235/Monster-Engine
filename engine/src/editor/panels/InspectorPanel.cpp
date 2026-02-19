#include "engine/editor/panels/InspectorPanel.h"

#include <imgui.h>
#include <entt.hpp>
#include <glm.hpp>
#include <gtc/type_ptr.hpp>

#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/editor/EditorLayer.h"

namespace se {

InspectorPanel::InspectorPanel(EditorLayer* editor) : editor_(editor) {}

void InspectorPanel::OnImGuiRender() {
    ImGui::Begin(GetName(), &open_);

    if (!scene_ || !editor_) {
        ImGui::TextDisabled("No scene or editor");
        ImGui::End();
        return;
    }

    uint32_t selectedHandle = editor_->GetSelectedEntity();
    auto& registry = scene_->GetRegistry();
    auto selectedEntity = static_cast<entt::entity>(selectedHandle);

    if (selectedHandle == UINT32_MAX || !registry.valid(selectedEntity)) {
        ImGui::TextDisabled("No entity selected");
        ImGui::End();
        return;
    }

    // ─── Name ────────────────────────────────────────────────────
    if (registry.any_of<NameComponent>(selectedEntity)) {
        auto& name = registry.get<NameComponent>(selectedEntity);
        char buffer[256];
        std::strncpy(buffer, name.Name.c_str(), sizeof(buffer));
        buffer[sizeof(buffer) - 1] = '\0';

        if (ImGui::InputText("Name", buffer, sizeof(buffer))) {
            name.Name = buffer;
        }
        ImGui::Separator();
    }

    // ─── Transform ───────────────────────────────────────────────
    if (registry.any_of<TransformComponent>(selectedEntity)) {
        DrawTransformComponent();
    }

    // ─── Camera ──────────────────────────────────────────────────
    if (registry.any_of<CameraComponent>(selectedEntity)) {
        DrawCameraComponent();
    }

    // ─── Spring Arm ──────────────────────────────────────────────
    if (registry.any_of<SpringArmComponent>(selectedEntity)) {
        DrawSpringArmComponent();
    }

    // ─── Directional Light ───────────────────────────────────────
    if (registry.any_of<DirectionalLightComponent>(selectedEntity)) {
        DrawDirectionalLightComponent();
    }

    // ─── Point Light ─────────────────────────────────────────────
    if (registry.any_of<PointLightComponent>(selectedEntity)) {
        DrawPointLightComponent();
    }

    ImGui::End();
}

void InspectorPanel::DrawTransformComponent() {
    auto& registry = scene_->GetRegistry();
    auto entity = static_cast<entt::entity>(editor_->GetSelectedEntity());
    auto& transform = registry.get<TransformComponent>(entity);

    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool changed = false;
        changed |= ImGui::DragFloat3("Position", glm::value_ptr(transform.Position), 0.1f);
        changed |= ImGui::DragFloat3("Rotation", glm::value_ptr(transform.Rotation), 1.0f);
        changed |= ImGui::DragFloat3("Scale", glm::value_ptr(transform.Scale), 0.05f, 0.01f, 100.0f);

        if (changed) {
            transform.MarkDirty();
        }
    }
}

void InspectorPanel::DrawCameraComponent() {
    auto& registry = scene_->GetRegistry();
    auto entity = static_cast<entt::entity>(editor_->GetSelectedEntity());
    auto& cam = registry.get<CameraComponent>(entity);

    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        // Mode selector
        const char* modes[] = {"FreeFly", "ThirdPerson", "Orbit", "Fixed"};
        int currentMode = static_cast<int>(cam.Mode);
        if (ImGui::Combo("Mode", &currentMode, modes, IM_ARRAYSIZE(modes))) {
            cam.Mode = static_cast<CameraMode>(currentMode);
        }

        ImGui::Checkbox("Is Main", &cam.IsMain);
        ImGui::DragFloat("FOV", &cam.FOV, 0.5f, 10.0f, 120.0f);
        ImGui::DragFloat("Near Plane", &cam.NearPlane, 0.01f, 0.001f, 10.0f);
        ImGui::DragFloat("Far Plane", &cam.FarPlane, 10.0f, 10.0f, 10000.0f);
        ImGui::DragFloat("Move Speed", &cam.MoveSpeed, 0.1f, 0.1f, 100.0f);
        ImGui::DragFloat("Sprint Speed", &cam.SprintSpeed, 0.1f, 0.1f, 200.0f);
        ImGui::DragFloat("Sensitivity X", &cam.LookSensitivityX, 0.01f, 0.01f, 2.0f);
        ImGui::DragFloat("Sensitivity Y", &cam.LookSensitivityY, 0.01f, 0.01f, 2.0f);
        ImGui::Checkbox("Invert Y", &cam.InvertY);

        ImGui::Separator();
        ImGui::TextDisabled("Yaw: %.1f | Pitch: %.1f", cam.Yaw, cam.Pitch);
    }
}

void InspectorPanel::DrawDirectionalLightComponent() {
    auto& registry = scene_->GetRegistry();
    auto entity = static_cast<entt::entity>(editor_->GetSelectedEntity());
    auto& light = registry.get<DirectionalLightComponent>(entity);

    if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::ColorEdit3("Color", glm::value_ptr(light.Color));
        ImGui::DragFloat("Intensity", &light.Intensity, 1000.0f, 0.0f, 500000.0f);
        ImGui::Checkbox("Enabled", &light.Enabled);
        ImGui::Checkbox("Cast Shadows", &light.CastShadows);
    }
}

void InspectorPanel::DrawPointLightComponent() {
    auto& registry = scene_->GetRegistry();
    auto entity = static_cast<entt::entity>(editor_->GetSelectedEntity());
    auto& light = registry.get<PointLightComponent>(entity);

    if (ImGui::CollapsingHeader("Point Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::ColorEdit3("Color", glm::value_ptr(light.Color));
        ImGui::DragFloat("Intensity", &light.Intensity, 1000.0f, 0.0f, 500000.0f);
        ImGui::DragFloat("Falloff", &light.Falloff, 0.5f, 0.1f, 100.0f);
        ImGui::Checkbox("Enabled", &light.Enabled);
    }
}

void InspectorPanel::DrawSpringArmComponent() {
    auto& registry = scene_->GetRegistry();
    auto entity = static_cast<entt::entity>(editor_->GetSelectedEntity());
    auto& arm = registry.get<SpringArmComponent>(entity);

    if (ImGui::CollapsingHeader("Spring Arm", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::DragFloat("Arm Length", &arm.TargetArmLength, 0.1f, 0.1f, 50.0f);
        ImGui::DragFloat3("Socket Offset", glm::value_ptr(arm.SocketOffset), 0.1f);
        ImGui::DragFloat("Pitch", &arm.Pitch, 1.0f, arm.MinPitch, arm.MaxPitch);
        ImGui::DragFloat("Yaw", &arm.Yaw, 1.0f);
        ImGui::DragFloat("Min Pitch", &arm.MinPitch, 1.0f, -89.0f, 0.0f);
        ImGui::DragFloat("Max Pitch", &arm.MaxPitch, 1.0f, 0.0f, 89.0f);
        ImGui::Checkbox("Collision Test", &arm.DoCollisionTest);
        ImGui::DragFloat("Probe Size", &arm.ProbeSize, 0.01f, 0.01f, 1.0f);

        ImGui::Separator();
        ImGui::TextDisabled("Current Arm Length: %.2f", arm.CurrentArmLength);
    }
}

}  // namespace se
