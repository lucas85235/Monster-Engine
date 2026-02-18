#pragma once
/**
 * EditorGrid.h - Renders a reference grid in the editor viewport.
 *
 * Uses a flat Filament renderable (unlit material) as a ground-plane
 * reference for the editor camera.
 */

#include "engine/renderer/MaterialHandle.h"
#include "engine/renderer/MeshSystem.h"

namespace filament {
class Scene;
}  // namespace filament

namespace mst {

class EditorGrid {
   public:
    EditorGrid() = default;
    ~EditorGrid();

    /**
     * Create the grid renderable and add it to the given Filament scene.
     * Must be called after the engine subsystems (MeshSystem, MaterialSystem) are initialized.
     */
    void Init();

    void SetVisible(bool visible) { visible_ = visible; }
    bool IsVisible() const { return visible_; }

   private:
    bool visible_ = true;
    se::RenderableHandle gridHandle_{};
    se::MaterialHandle gridMaterial_{};
};

}  // namespace mst
