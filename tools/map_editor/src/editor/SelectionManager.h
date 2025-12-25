#pragma once
/**
 * SelectionManager.h - Entity selection state for the editor.
 *
 * Tracks selected entities and provides selection manipulation APIs.
 * Supports single and multi-selection with toggle capability.
 */

#include <vector>

#include "engine/ecs/Entity.h"

namespace mst {

class SelectionManager {
   public:
    void Select(se::Entity entity);
    void AddToSelection(se::Entity entity);
    void RemoveFromSelection(se::Entity entity);
    void ClearSelection();
    void ToggleSelection(se::Entity entity);

    bool IsSelected(se::Entity entity) const;
    bool HasSelection() const;

    se::Entity                       GetPrimarySelection() const;
    const std::vector<se::Entity>&   GetSelectedEntities() const;
    std::vector<se::Entity>&         GetSelectedEntities();

   private:
    std::vector<se::Entity> selectedEntities_;
};

}  // namespace mst
