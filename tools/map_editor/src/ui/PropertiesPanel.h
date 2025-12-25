#pragma once

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
