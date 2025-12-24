#include "editor/GizmoController.h"

#include <ImGuizmo.h>
#include <glm.hpp>
#include <gtc/type_ptr.hpp>

#include "engine/Log.h"

namespace mst {

void GizmoController::SetOperation(Operation op) {
    if (operation_ != op) {
        operation_ = op;
        SE_LOG_INFO("GizmoController: Operation set to {}", GetOperationName());
    }
}

void GizmoController::SetSpace(Space space) {
    if (space_ != space) {
        space_ = space;
        SE_LOG_INFO("GizmoController: Space set to {}", GetSpaceName());
    }
}

void GizmoController::ToggleSpace() {
    space_ = (space_ == Space::World) ? Space::Local : Space::World;
    SE_LOG_INFO("GizmoController: Space toggled to {}", GetSpaceName());
}

void GizmoController::CycleOperation() {
    switch (operation_) {
        case Operation::Translate: operation_ = Operation::Rotate; break;
        case Operation::Rotate: operation_ = Operation::Scale; break;
        case Operation::Scale: operation_ = Operation::Translate; break;
    }
    SE_LOG_INFO("GizmoController: Operation cycled to {}", GetOperationName());
}

bool GizmoController::Manipulate(const Camera& camera, float aspectRatio,
                                  se::TransformComponent& transform) {
    ImGuizmo::OPERATION imguizmoOp = ImGuizmo::TRANSLATE;
    switch (operation_) {
        case Operation::Translate: imguizmoOp = ImGuizmo::TRANSLATE; break;
        case Operation::Rotate: imguizmoOp = ImGuizmo::ROTATE; break;
        case Operation::Scale: imguizmoOp = ImGuizmo::SCALE; break;
    }

    ImGuizmo::MODE imguizmoMode =
        (space_ == Space::World) ? ImGuizmo::WORLD : ImGuizmo::LOCAL;

    Matrix4 view       = camera.getViewMatrix();
    Matrix4 projection = camera.getProjectionMatrix(aspectRatio);
    Matrix4 model      = transform.GetTransform();

    ImGuizmo::SetOrthographic(false);
    // Note: SetDrawlist and SetRect are called in MapEditorLayer::RenderViewport

    float deltaMatrix[16];
    float snapValues[3] = {0.0f, 0.0f, 0.0f};

    bool manipulated = ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(projection),
                                             imguizmoOp, imguizmoMode, glm::value_ptr(model),
                                             deltaMatrix, nullptr);

    isUsing_ = ImGuizmo::IsUsing();

    if (manipulated && isUsing_) {
        Vector3 position, rotation, scale;
        ImGuizmo::DecomposeMatrixToComponents(glm::value_ptr(model), glm::value_ptr(position),
                                               glm::value_ptr(rotation), glm::value_ptr(scale));

        transform.SetPosition(position);
        transform.SetRotation(rotation);
        transform.SetScale(scale);

        return true;
    }

    return false;
}

const char* GizmoController::GetOperationName() const {
    switch (operation_) {
        case Operation::Translate: return "Translate";
        case Operation::Rotate: return "Rotate";
        case Operation::Scale: return "Scale";
        default: return "Unknown";
    }
}

const char* GizmoController::GetSpaceName() const {
    return (space_ == Space::World) ? "World" : "Local";
}

}  // namespace mst
