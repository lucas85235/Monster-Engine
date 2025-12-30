#pragma once
/**
 * MapEditorLayer.h - Orchestrator layer for the map editor.
 *
 * Lightweight coordinator that initializes subsystems and delegates
 * work to specialized components. Follows the Facade pattern.
 */

#include <memory>
#include <vector>

#include "core/EditorContext.h"
#include "core/InputHandler.h"
#include "ui/IPanel.h"
#include "ui/FileDialogManager.h"
#include "ui/ViewportPanel.h"
#include "ui/StatusBarPanel.h"
#include "ui/HierarchyPanel.h"
#include "ui/MainMenuBar.h"
#include "ui/PropertiesPanel.h"
#include "engine/Layer.h"

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
    void SetupEventHandlers();
    void SetupDockspace();
    void ProcessCameraInput(float ts);
    void ProcessMenuActions(const MenuBarActions& actions);
    void ProcessHierarchyActions();
    
    Scope<EditorContext> context_;
    InputHandler inputHandler_;
    
    // UI Components
    MainMenuBar menuBar_;
    Scope<ViewportPanel> viewportPanel_;
    Scope<StatusBarPanel> statusBarPanel_;
    HierarchyPanel hierarchyPanel_;
    PropertiesPanel propertiesPanel_;
    FileDialogManager fileDialogs_;
    
    // Input state
    float lastMouseX_ = 0.0f;
    float lastMouseY_ = 0.0f;
    
    // Panel visibility (for View menu)
    bool viewportVisible_ = true;
    bool hierarchyVisible_ = true;
    bool propertiesVisible_ = true;
    bool statusBarVisible_ = true;
};

}  // namespace mst
