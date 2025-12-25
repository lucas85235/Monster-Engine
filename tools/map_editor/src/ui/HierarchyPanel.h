#pragma once
/**
 * HierarchyPanel.h - Scene entity tree view panel.
 *
 * Displays all entities in a selectable list with context menu support
 * for delete and duplicate operations.
 */


#include "editor/SelectionManager.h"
#include "engine/ecs/Scene.h"

namespace mst {

class HierarchyPanel {
   public:
    void Render(se::Scene& scene, SelectionManager& selection);

    bool WantsDelete() const { return wantsDelete_; }
    bool WantsDuplicate() const { return wantsDuplicate_; }
    void ClearActions();

   private:
    bool wantsDelete_    = false;
    bool wantsDuplicate_ = false;

    void RenderEntityNode(se::Entity entity, SelectionManager& selection);
    void RenderContextMenu(se::Entity entity, SelectionManager& selection);
};

}  // namespace mst
