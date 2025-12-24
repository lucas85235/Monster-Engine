#pragma once

#include <memory>
#include <string>

#include "core/MapData.h"
#include "core/MapSerializer.h"
#include "core/PrimitiveFactory.h"
#include "editor/EditorCamera.h"
#include "editor/GizmoController.h"
#include "editor/SelectionManager.h"
#include "engine/Layer.h"
#include "engine/ecs/Scene.h"
#include "ui/HierarchyPanel.h"
#include "ui/MainMenuBar.h"
#include "ui/PropertiesPanel.h"

namespace mst {

class MapEditorLayer : public se::Layer {
   public:
    MapEditorLayer();
    ~MapEditorLayer() override;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float ts) override;
    void OnRender() override;
    void OnImGuiRender() override;

   private:
    // Scene
    Scope<se::Scene> scene_;
    EditorCamera     editorCamera_;

    // Selection & gizmo
    SelectionManager selection_;
    GizmoController  gizmo_;

    // UI
    MainMenuBar     menuBar_;
    HierarchyPanel  hierarchy_;
    PropertiesPanel properties_;

    // Map data
    MapData     currentMap_;
    std::string currentFilePath_;
    bool        showExportDialog_ = false;
    char        exportFileName_[256] = "untitled";

    // Grid
    bool showGrid_ = true;

    // Input state
    float lastMouseX_ = 0.0f;
    float lastMouseY_ = 0.0f;

    void ProcessMenuActions(const MenuBarActions& actions);
    void ProcessHierarchyActions();
    void ProcessKeyboardShortcuts();

    void CreatePrimitive(PrimitiveType type);
    void DuplicateSelected();
    void DeleteSelected();

    void ShowExportDialog();
    void ExportMap(const std::string& filename);

    void BuildMapData();
    void RenderGrid();
    void RenderViewport();
    void RenderStatusBar();
};

}  // namespace mst
