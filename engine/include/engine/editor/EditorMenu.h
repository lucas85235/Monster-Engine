#pragma once

#include <functional>
#include <map>
#include <string>
#include <vector>

namespace se {

class Scene;

/**
 * Describes a single menu item that can be registered into the editor menu bar.
 *
 * Items are organized hierarchically by their Category path.
 * Example categories:
 *   - "Entity/Primitives"  → Menu "Entity" > Submenu "Primitives"
 *   - "Entity/Lights"      → Menu "Entity" > Submenu "Lights"
 *   - "Entity"             → Menu "Entity" (top-level item)
 */
struct MenuItem {
    std::string Label;      // Display text, e.g. "Cube"
    std::string Category;   // Hierarchical path, e.g. "Entity/Primitives"
    std::function<void(Scene&)> Action;  // Callback when clicked
    std::string Shortcut;   // Optional keyboard shortcut display text
};

/**
 * Registration-based menu system for the editor.
 *
 * Usage:
 *   // During initialization (e.g. in EditorLayer::OnAttach)
 *   EditorMenu::Register({"Cube", "Entity/Primitives", [](Scene& s) {
 *       auto e = s.CreateEntity("Cube");
 *       // ... add mesh components ...
 *   }});
 *
 * Items are rendered automatically by EditorLayer in the main menu bar.
 * Adding new items is a single Register() call — no other files need modification.
 */
class EditorMenu {
   public:
    /** Register a menu item. Can be called at any time. */
    static void Register(MenuItem item);

    /** Clear all registered items. Called on EditorLayer shutdown. */
    static void Clear();

    /**
     * Render all registered menu items as ImGui menus.
     * Called by EditorLayer during MainMenuBar rendering.
     */
    static void RenderMenuItems(Scene* scene);

   private:
    // Category -> list of items in that category
    static std::map<std::string, std::vector<MenuItem>>& GetRegistry();
};

}  // namespace se
