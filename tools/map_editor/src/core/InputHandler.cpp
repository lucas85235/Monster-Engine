#include "InputHandler.h"

#include "EditorContext.h"
#include "engine/input/InputManager.h"
#include "engine/core/Log.h"
#include "engine/ecs/SimpleComponents.h"

#include <GLFW/glfw3.h>

using namespace se;

namespace mst {

void InputHandler::ProcessShortcuts(EditorContext& ctx, bool viewportHovered, bool viewportFocused) {
    if (!enabled_) return;
    
    viewportActive_ = viewportHovered || viewportFocused;
    
    ProcessGizmoShortcuts(ctx);
    ProcessEntityShortcuts(ctx);
    ProcessViewShortcuts(ctx);
    ProcessFileShortcuts(ctx);
}

void InputHandler::ProcessGizmoShortcuts(EditorContext& ctx) {
    if (!viewportActive_) return;
    if (IsCtrlDown()) return;
    
    auto& gizmo = ctx.GetGizmo();
    
    if (IsKeyDown(GLFW_KEY_W)) {
        gizmo.SetOperation(GizmoController::Operation::Translate);
    }
    if (IsKeyDown(GLFW_KEY_E)) {
        gizmo.SetOperation(GizmoController::Operation::Rotate);
    }
    if (IsKeyDown(GLFW_KEY_R)) {
        gizmo.SetOperation(GizmoController::Operation::Scale);
    }
    if (IsKeyJustPressed(GLFW_KEY_Q)) {
        gizmo.ToggleSpace();
    }
}

void InputHandler::ProcessEntityShortcuts(EditorContext& ctx) {
    auto& selection = ctx.GetSelection();
    auto& entityManager = ctx.GetEntityManager();
    
    if (IsKeyJustPressed(GLFW_KEY_DELETE)) {
        if (selection.HasSelection()) {
            auto entities = selection.GetSelectedEntities();
            selection.ClearSelection();
            entityManager.DeleteEntities(entities);
            ctx.GetDocument().MarkDirty();
        }
    }
    
    if (IsCtrlDown() && IsKeyJustPressed(GLFW_KEY_D)) {
        if (selection.HasSelection()) {
            auto entity = selection.GetPrimarySelection();
            auto duplicate = entityManager.DuplicateEntity(entity);
            if (duplicate.IsValid()) {
                selection.Select(duplicate);
                ctx.GetDocument().MarkDirty();
            }
        }
    }
    
    if (IsKeyJustPressed(GLFW_KEY_F) && selection.HasSelection()) {
        auto entity = selection.GetPrimarySelection();
        if (entity.IsValid() && entity.HasComponent<se::TransformComponent>()) {
            auto& transform = entity.GetComponent<se::TransformComponent>();
            ctx.GetCamera().FocusOnPoint(transform.Position);
            SE_LOG_INFO("InputHandler: Camera focused on entity");
        }
    }
    
    if (IsKeyJustPressed(GLFW_KEY_ESCAPE)) {
        selection.ClearSelection();
    }
}

void InputHandler::ProcessViewShortcuts(EditorContext& ctx) {
    // Grid and collider toggles handled via events in the new architecture
}

void InputHandler::ProcessFileShortcuts(EditorContext& ctx) {
    if (IsCtrlDown() && IsKeyJustPressed(GLFW_KEY_Z)) {
        ctx.GetCommandSystem().Undo();
    }
    
    if (IsCtrlDown() && IsKeyJustPressed(GLFW_KEY_Y)) {
        ctx.GetCommandSystem().Redo();
    }
    
    if (IsCtrlDown() && IsKeyDown(GLFW_KEY_LEFT_SHIFT) && IsKeyJustPressed(GLFW_KEY_Z)) {
        ctx.GetCommandSystem().Redo();
    }
}

bool InputHandler::IsKeyJustPressed(int key) {
    bool currentState = IsKeyDown(key);
    bool previousState = previousKeyStates_[key];
    previousKeyStates_[key] = currentState;
    
    return currentState && !previousState;
}

bool InputHandler::IsKeyDown(int key) const {
    return se::InputManager::Get().IsKeyDown(key);
}

bool InputHandler::IsCtrlDown() const {
    return IsKeyDown(GLFW_KEY_LEFT_CONTROL) || IsKeyDown(GLFW_KEY_RIGHT_CONTROL);
}

}  // namespace mst
