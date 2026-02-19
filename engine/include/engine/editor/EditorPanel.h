#pragma once

#include <string>

namespace se {

class Scene;

/**
 * Base class for all editor panels.
 *
 * Subclass this to create new editor panels. Each panel gets its own
 * ImGui window and can be toggled open/closed via the View menu.
 *
 * The EditorLayer manages panel lifetime and calls OnImGuiRender()
 * each frame for open panels.
 */
class EditorPanel {
   public:
    virtual ~EditorPanel() = default;

    /**
     * Render the panel content. Called each frame when the panel is open.
     * The ImGui::Begin/End window is managed by the panel itself, allowing
     * full control over window flags.
     */
    virtual void OnImGuiRender() = 0;

    /** Display name shown in the window title and View menu. */
    virtual const char* GetName() const = 0;

    bool IsOpen() const { return open_; }
    void SetOpen(bool open) { open_ = open; }
    void ToggleOpen() { open_ = !open_; }

    void SetScene(Scene* scene) { scene_ = scene; }
    Scene* GetScene() const { return scene_; }

   protected:
    bool   open_  = true;
    Scene* scene_ = nullptr;
};

}  // namespace se
