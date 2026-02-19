#pragma once

#include "engine/editor/EditorPanel.h"

namespace se {

class EditorLayer;

/**
 * Inspector panel — displays and edits components of the selected entity.
 *
 * Supports editing:
 * - TransformComponent (position, rotation, scale)
 * - CameraComponent (mode, FOV, speeds)
 * - DirectionalLightComponent (color, intensity, shadows)
 * - PointLightComponent (color, intensity, falloff)
 * - SpringArmComponent (arm length, offset, pitch limits)
 * - NameComponent (rename)
 */
class InspectorPanel : public EditorPanel {
   public:
    explicit InspectorPanel(EditorLayer* editor);

    void OnImGuiRender() override;
    const char* GetName() const override { return "Inspector"; }

   private:
    void DrawTransformComponent();
    void DrawCameraComponent();
    void DrawDirectionalLightComponent();
    void DrawPointLightComponent();
    void DrawSpringArmComponent();

    EditorLayer* editor_ = nullptr;
};

}  // namespace se
