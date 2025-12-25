#pragma once

#include "IPanel.h"

#include <imgui.h>

namespace ued {

struct UIWidgetNode;

class HierarchyPanel : public IPanel {
public:
    void Render(UIEditorContext& ctx) override;
    const char* GetName() const override { return "Hierarchy"; }

private:
    void RenderNode(UIEditorContext& ctx, UIWidgetNode* node);
};

}  // namespace ued
