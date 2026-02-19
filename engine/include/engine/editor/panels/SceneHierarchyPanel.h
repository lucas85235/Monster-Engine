#pragma once

#include "engine/editor/EditorPanel.h"

namespace se {

class EditorLayer;

/**
 * Scene hierarchy panel — displays all entities in the scene as a list.
 *
 * Features:
 * - Click to select entity (shared with InspectorPanel via EditorLayer)
 * - Right-click context menu: Delete Entity
 * - Search/filter by name
 */
class SceneHierarchyPanel : public EditorPanel {
   public:
    explicit SceneHierarchyPanel(EditorLayer* editor);

    void OnImGuiRender() override;
    const char* GetName() const override { return "Scene Hierarchy"; }

   private:
    void DrawEntityNode(uint32_t entityHandle, const char* name);
    void DrawContextMenu();

    EditorLayer* editor_ = nullptr;
    char searchBuffer_[128] = {0};
};

}  // namespace se
