#pragma once
/**
 * MaterialEditorPanel.h - Material Editor UI panel.
 *
 * Provides a separate window for creating, editing, and managing PBR materials.
 * Features:
 * - Material library (list of loaded materials)
 * - Full PBR parameter controls
 * - Texture slot management with file loading
 * - Save/Load material files
 */

#include <memory>
#include <string>
#include <vector>

#include "core/EditorMaterialData.h"

namespace mst {

class EditorContext;

class MaterialEditorPanel {
   public:
    MaterialEditorPanel() = default;
    ~MaterialEditorPanel() = default;

    void Render(EditorContext& context);

    bool IsVisible() const { return visible_; }
    void SetVisible(bool visible) { visible_ = visible; }
    void ToggleVisible() { visible_ = !visible_; }

    void SelectMaterial(int index) { selectedMaterialIndex_ = index; }
    int GetSelectedMaterialIndex() const { return selectedMaterialIndex_; }

   private:
    void RenderMaterialLibrary(EditorContext& context);
    void RenderPBRParameters(EditorMaterialData& material);
    void RenderTextureSlots(EditorMaterialData& material, EditorContext& context);
    void RenderAdvancedParameters(EditorMaterialData& material);
    void RenderActions(EditorContext& context);

    bool visible_ = false;
    int selectedMaterialIndex_ = -1;
    
    char searchBuffer_[256] = "";
    bool showAdvancedParams_ = false;
    
    // Pending texture load state (for file dialog callback)
    std::string* pendingTexturePath_ = nullptr;
    bool* pendingTextureUse_ = nullptr;
};

}  // namespace mst
