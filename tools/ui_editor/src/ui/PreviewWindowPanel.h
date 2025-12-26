#pragma once

#include "IPanel.h"

#include <imgui.h>
#include <glad/glad.h>

#include "Engine.h"
#include "editor/UIPreviewRenderer.h"

namespace ued {

class PreviewWindowPanel : public IPanel {
public:
    PreviewWindowPanel();
    ~PreviewWindowPanel();
    
    void Render(UIEditorContext& ctx) override;
    const char* GetName() const override { return "RmlUI Preview"; }

private:
    se::Scope<UIPreviewRenderer> previewRenderer_;
    ImVec2 lastSize_ = {800, 600};
};

}  // namespace ued

