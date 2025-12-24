#pragma once

#include <memory>
#include <string>

#include "core/MapData.h"
#include "core/MapSerializer.h"
#include "core/PrimitiveFactory.h"
#include "editor/EditorCamera.h"
#include "editor/EditorFramebuffer.h"
#include "editor/EditorGrid.h"
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

    // Framebuffer for viewport rendering
    Scope<EditorFramebuffer> framebuffer_;
    Scope<EditorGrid> editorGrid_;
    uint32_t viewportWidth_ = 1280;
    uint32_t viewportHeight_ = 720;

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
    bool        showOpenDialog_ = false;
    char        exportFileName_[256] = "untitled";
    char        openFileName_[256] = "";

    // Grid
    bool showGrid_ = true;  // Press G to toggle grid

    // Viewport state
    bool viewportHovered_ = false;
    bool viewportFocused_ = false;

    // Input state
    float lastMouseX_ = 0.0f;
    float lastMouseY_ = 0.0f;
    
    // Key state tracking for one-shot actions
    bool wasKeyFPressed_ = false;
    bool wasKeyGPressed_ = false;
    bool wasKeyDeletePressed_ = false;

    void ProcessMenuActions(const MenuBarActions& actions);
    void ProcessHierarchyActions();
    void ProcessKeyboardShortcuts();

    void CreatePrimitive(PrimitiveType type);
    void DuplicateSelected();
    void DeleteSelected();

    void ShowExportDialog();
    void ShowOpenDialog();
    void ExportMap(const std::string& filename);
    void LoadMap(const std::string& filename);

    void BuildMapData();
    void RenderGrid(const Matrix4& view, const Matrix4& projection);
    void RenderViewport();
    void RenderStatusBar();
};

}  // namespace mst
