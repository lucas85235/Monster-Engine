#include "ViewportPanel.h"

#include <ImGuizmo.h>
#include <glad/glad.h>

#include "../commands/EntityCommands.h"
#include "../core/EditorContext.h"
#include "engine/Log.h"
#include "engine/ecs/SimpleComponents.h"

using namespace se;

namespace mst {

ViewportPanel::ViewportPanel() {
    renderer_ = se::CreateScope<ViewportRenderer>(1280, 720);
}

void ViewportPanel::Render(EditorContext& ctx) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
    ImGui::Begin(GetName());
    
    isHovered_ = ImGui::IsWindowHovered();
    isFocused_ = ImGui::IsWindowFocused();
    
    ImVec2 viewportSize = ImGui::GetContentRegionAvail();
    
    if (viewportSize.x > 0 && viewportSize.y > 0) {
        uint32_t newWidth = static_cast<uint32_t>(viewportSize.x);
        uint32_t newHeight = static_cast<uint32_t>(viewportSize.y);
        
        renderer_->Resize(newWidth, newHeight);
        
        uint64_t textureId = renderer_->GetColorAttachment();
        ImGui::Image(reinterpret_cast<void*>(textureId), viewportSize, 
                     ImVec2(0, 1), ImVec2(1, 0));
        
        position_ = ImGui::GetItemRectMin();
        size_ = viewportSize;
        
        ImGuizmo::SetDrawlist();
        ImGuizmo::SetRect(position_.x, position_.y, size_.x, size_.y);
        
        RenderGizmo(ctx);
        ProcessMousePicking(ctx);
    }
    
    ImGui::End();
    ImGui::PopStyleVar();
}

void ViewportPanel::RenderGizmo(EditorContext& ctx) {
    auto& selection = ctx.GetSelection();
    if (!selection.HasSelection()) return;
    
    auto entity = selection.GetPrimarySelection();
    if (!entity.IsValid() || !entity.HasComponent<se::TransformComponent>()) return;
    
    auto& transform = entity.GetComponent<se::TransformComponent>();
    auto& gizmo = ctx.GetGizmo();
    float aspectRatio = renderer_->GetAspectRatio();
    
    // Store original transform before manipulation starts
    if (ImGuizmo::IsOver() && ImGui::IsMouseClicked(0)) {
        gizmo.BeginManipulation(entity, transform);
    }
    
    bool manipulated = gizmo.Manipulate(
        ctx.GetCamera().GetCamera(), aspectRatio, transform);
    
    if (manipulated) {
        ctx.GetDocument().MarkDirty();
    }
    
    // Create undo command when manipulation ends
    if (gizmo.EndedManipulation()) {
        auto manipulatedEntity = gizmo.GetManipulatedEntity();
        if (manipulatedEntity.IsValid()) {
            ctx.GetCommandSystem().Execute(
                std::make_unique<TransformEntityCommand>(
                    ctx, manipulatedEntity,
                    gizmo.GetOriginalTransform(),
                    transform));
        }
        gizmo.ClearEndedFlag();
    }
}

void ViewportPanel::ProcessMousePicking(EditorContext& ctx) {
    if (!isHovered_) return;
    
    bool isLeftMouseDown = ImGui::IsMouseDown(ImGuiMouseButton_Left);
    bool leftMouseJustPressed = isLeftMouseDown && !wasMousePressed_;
    wasMousePressed_ = isLeftMouseDown;
    
    if (!leftMouseJustPressed) return;
    if (ImGuizmo::IsOver()) return;
    
    ImVec2 mousePos = ImGui::GetMousePos();
    float relX = mousePos.x - position_.x;
    float relY = mousePos.y - position_.y;
    
    if (relX < 0 || relY < 0 || relX > size_.x || relY > size_.y) return;
    
    se::Entity pickedEntity = picker_.Pick(
        ctx.GetCamera().GetCamera(),
        ctx.GetScene(),
        relX, relY,
        size_.x, size_.y
    );
    
    auto& selection = ctx.GetSelection();
    
    if (pickedEntity.IsValid()) {
        selection.ClearSelection();
        selection.Select(pickedEntity);
        SE_LOG_INFO("ViewportPanel: Picked entity '{}'", 
                    pickedEntity.GetComponent<se::NameComponent>().Name);
    } else {
        selection.ClearSelection();
    }
}

void ViewportPanel::ToggleGrid() {
    auto& settings = renderer_->GetSettings();
    settings.showGrid = !settings.showGrid;
    SE_LOG_INFO("ViewportPanel: Grid {}", settings.showGrid ? "ON" : "OFF");
}

void ViewportPanel::ToggleColliderDebug() {
    auto& settings = renderer_->GetSettings();
    settings.showColliderDebug = !settings.showColliderDebug;
    SE_LOG_INFO("ViewportPanel: Collider Debug {}", settings.showColliderDebug ? "ON" : "OFF");
}

}  // namespace mst
