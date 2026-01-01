#pragma once
/**
 * PropertiesPanel.h - Entity inspector panel.
 *
 * Displays and edits transform, mesh renderer, collision properties,
 * and material assignment for the selected entity. Includes gizmo mode controls.
 */


#include "core/MapData.h"
#include "core/PrimitiveFactory.h"
#include "editor/GizmoController.h"
#include "editor/SelectionManager.h"
#include "engine/ecs/Entity.h"

namespace mst {

class EditorContext;

class PropertiesPanel {
   public:
    void Render(SelectionManager& selection, GizmoController& gizmo, EditorContext& context);

   private:
    void RenderTransform(se::TransformComponent& transform);
    void RenderMeshRenderer(se::MeshRenderComponent& mesh);
    void RenderEditorMetadata(PrimitiveFactory::EditorMetadata& metadata);
    void RenderGizmoControls(GizmoController& gizmo);
    void RenderMaterial(PrimitiveFactory::EditorMetadata& metadata, EditorContext& context, se::Entity entity);
};

}  // namespace mst


