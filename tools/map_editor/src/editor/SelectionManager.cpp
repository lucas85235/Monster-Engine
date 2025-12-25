#include "editor/SelectionManager.h"

#include <algorithm>

#include "engine/Log.h"

namespace mst {

void SelectionManager::Select(se::Entity entity) {
    ClearSelection();
    if (entity.IsValid()) {
        selectedEntities_.push_back(entity);
        SE_LOG_INFO("SelectionManager: Selected entity {}", entity.GetID());
    }
}

void SelectionManager::AddToSelection(se::Entity entity) {
    if (!entity.IsValid()) return;

    if (!IsSelected(entity)) {
        selectedEntities_.push_back(entity);
        SE_LOG_INFO("SelectionManager: Added entity {} to selection", entity.GetID());
    }
}

void SelectionManager::RemoveFromSelection(se::Entity entity) {
    auto it = std::find(selectedEntities_.begin(), selectedEntities_.end(), entity);
    if (it != selectedEntities_.end()) {
        selectedEntities_.erase(it);
        SE_LOG_INFO("SelectionManager: Removed entity {} from selection", entity.GetID());
    }
}

void SelectionManager::ClearSelection() {
    if (!selectedEntities_.empty()) {
        SE_LOG_INFO("SelectionManager: Cleared selection ({} entities)", selectedEntities_.size());
        selectedEntities_.clear();
    }
}

void SelectionManager::ToggleSelection(se::Entity entity) {
    if (IsSelected(entity)) {
        RemoveFromSelection(entity);
    } else {
        AddToSelection(entity);
    }
}

bool SelectionManager::IsSelected(se::Entity entity) const {
    return std::find(selectedEntities_.begin(), selectedEntities_.end(), entity) !=
           selectedEntities_.end();
}

bool SelectionManager::HasSelection() const {
    return !selectedEntities_.empty();
}

se::Entity SelectionManager::GetPrimarySelection() const {
    if (selectedEntities_.empty()) {
        return se::Entity();
    }
    return selectedEntities_.front();
}

const std::vector<se::Entity>& SelectionManager::GetSelectedEntities() const {
    return selectedEntities_;
}

std::vector<se::Entity>& SelectionManager::GetSelectedEntities() {
    return selectedEntities_;
}

}  // namespace mst
