#include "PreviewWindowPanel.h"

#include "core/UIEditorContext.h"
#include "engine/core/Log.h"

namespace ued {

PreviewWindowPanel::PreviewWindowPanel() {
    previewRenderer_ = se::CreateScope<UIPreviewRenderer>();
}

PreviewWindowPanel::~PreviewWindowPanel() {
    previewRenderer_.reset();
}

void PreviewWindowPanel::Render(UIEditorContext& ctx) {
    if (!IsVisible()) return;
    
    ImGui::SetNextWindowSize(ImVec2(800, 600), ImGuiCond_FirstUseEver);
    
    ImGui::Begin(GetName(), nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    
    ImVec2 availSize = ImGui::GetContentRegionAvail();
    
    if (!previewRenderer_) {
        ImGui::Text("Preview renderer not available");
        ImGui::End();
        return;
    }
    
    // Initialize renderer if needed
    if (!previewRenderer_->IsInitialized() && availSize.x > 0 && availSize.y > 0) {
        previewRenderer_->Initialize((int)availSize.x, (int)availSize.y);
    }
    
    if (!previewRenderer_->IsInitialized()) {
        ImGui::Text("Preview not initialized yet...");
        ImGui::End();
        return;
    }
    
    // Update viewport if size changed
    if (availSize.x != lastSize_.x || availSize.y != lastSize_.y) {
        previewRenderer_->SetViewport((int)availSize.x, (int)availSize.y);
        lastSize_ = availSize;
    }
    
    // Update content
    previewRenderer_->UpdateFromWidgetTree(
        ctx.GetWidgetTree(), 
        ctx.GetDocument().GetStyleContent()
    );
    
    // Render to texture
    previewRenderer_->Render();
    
    // Display texture
    GLuint textureId = previewRenderer_->GetTextureId();
    if (textureId != 0) {
        ImGui::Image((ImTextureID)(intptr_t)textureId, availSize, ImVec2(0, 1), ImVec2(1, 0));
    } else {
        ImGui::Text("Texture ID is 0");
    }
    
    ImGui::End();
}

}  // namespace ued

