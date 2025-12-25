#pragma once

#include "IPanel.h"

#include <imgui.h>
#include <string>

namespace ued {

struct UIWidgetNode;

class PropertyEditorPanel : public IPanel {
public:
    void Render(UIEditorContext& ctx) override;
    const char* GetName() const override { return "Properties"; }

private:
    void RenderAttributeEditor(UIEditorContext& ctx, UIWidgetNode* node);
    void RenderStyleEditor(UIEditorContext& ctx, UIWidgetNode* node);
    void RenderAlignmentControls(UIEditorContext& ctx, UIWidgetNode* node);
    
    char idBuffer_[128] = "";
    char textBuffer_[512] = "";
    char classBuffer_[256] = "";
};

}  // namespace ued
