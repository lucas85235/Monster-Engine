#include "ParametersPanel.h"
#include "../core/EditorContext.h"

#include <imgui.h>

ParametersPanel::ParametersPanel(EditorContext& context) : context_(context) {
}

ParametersPanel::~ParametersPanel() = default;

void ParametersPanel::Render() {
    ImGui::Begin("Parameters");

    ImGui::Text("Graph Parameters");
    ImGui::Separator();

    static float speed = 0.0f;
    static float direction = 0.0f;
    static bool isJumping = false;
    static bool isAiming = false;

    ImGui::DragFloat("Speed", &speed, 0.01f, 0.0f, 10.0f);
    ImGui::DragFloat("Direction", &direction, 1.0f, -180.0f, 180.0f);
    ImGui::Checkbox("IsJumping", &isJumping);
    ImGui::Checkbox("IsAiming", &isAiming);

    ImGui::Separator();
    ImGui::Text("Add Parameter");

    ImGui::InputText("Name", newParamName_, sizeof(newParamName_));

    const char* types[] = { "Float", "Bool", "Int", "Trigger" };
    ImGui::Combo("Type", &newParamType_, types, IM_ARRAYSIZE(types));

    if (ImGui::Button("Add")) {
        if (newParamName_[0] != '\0') {
            newParamName_[0] = '\0';
        }
    }

    ImGui::End();
}
