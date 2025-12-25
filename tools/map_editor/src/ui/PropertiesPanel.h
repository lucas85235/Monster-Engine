#pragma once
/**
 * PropertiesPanel.h - Entity inspector panel.
 *
 * Displays and edits transform, mesh renderer, and collision properties
 * for the selected entity. Includes gizmo mode controls.
 */


#include "core/MapData.h"
#include "core/PrimitiveFactory.h"
#include "editor/GizmoController.h"
#include "editor/SelectionManager.h"

namespace mst {

class PropertiesPanel {
   public:
    void Render(SelectionManager& selection, GizmoController& gizmo);

   private:
    void RenderTransform(se::TransformComponent& transform);
    void RenderMeshRenderer(se::MeshRenderComponent& mesh);
    void RenderEditorMetadata(PrimitiveFactory::EditorMetadata& metadata);
    void RenderGizmoControls(GizmoController& gizmo);
};

}  // namespace mst
