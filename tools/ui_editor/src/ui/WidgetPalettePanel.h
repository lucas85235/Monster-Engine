#pragma once

#include "IPanel.h"

#include <imgui.h>
#include <string>
#include <vector>

namespace ued {

struct WidgetDefinition {
    std::string name;
    std::string type;
    std::string icon;
    std::string category;
};

class WidgetPalettePanel : public IPanel {
public:
    WidgetPalettePanel();
    
    void Render(UIEditorContext& ctx) override;
    const char* GetName() const override { return "Widget Palette"; }

private:
    void RenderCategory(UIEditorContext& ctx, const std::string& category);
    
    std::vector<WidgetDefinition> widgets_;
    std::string draggedWidgetType_;
};

}  // namespace ued
