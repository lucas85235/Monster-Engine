#pragma once

#include <memory>
#include <vector>

#include "engine/Layer.h"

namespace se {

class EditorPanel;
class Scene;

/**
 * Engine editor overlay layer.
 *
 * Sets up an ImGui DockSpace covering the full window, a main menu bar
 * with extensible menus (via EditorMenu), and a collection of EditorPanels.
 *
 * Usage:
 *   app.PushOverlay<se::EditorLayer>();
 *   // later, after creating a scene:
 *   editorLayer->SetScene(scene.get());
 *
 * The editor is completely separate from game logic — game layers
 * should NOT contain any ImGui code when using the editor.
 */
class EditorLayer : public Layer {
   public:
    EditorLayer();
    ~EditorLayer() override;

    void OnAttach() override;
    void OnDetach() override;
    void OnImGuiRender() override;

    /** Set the active scene for all panels and the entity creation menu. */
    void SetScene(Scene* scene);
    Scene* GetScene() const { return scene_; }

    /** Add a panel to the editor. Ownership is transferred. */
    void AddPanel(std::unique_ptr<EditorPanel> panel);

    /** Get selected entity handle (shared between Hierarchy and Inspector). */
    uint32_t GetSelectedEntity() const { return selectedEntity_; }
    void     SetSelectedEntity(uint32_t entity) { selectedEntity_ = entity; }

   private:
    void SetupDockSpace();
    void RenderMainMenuBar();
    void RenderViewMenu();
    void RegisterDefaultMenuItems();

    Scene* scene_ = nullptr;
    std::vector<std::unique_ptr<EditorPanel>> panels_;

    // Shared editor state
    uint32_t selectedEntity_ = UINT32_MAX;  // entt::null equivalent
    bool     showDemoWindow_  = false;
};

}  // namespace se
