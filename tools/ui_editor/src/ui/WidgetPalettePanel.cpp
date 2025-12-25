#include "WidgetPalettePanel.h"

#include "core/UIEditorContext.h"

namespace ued {

WidgetPalettePanel::WidgetPalettePanel() {
    // Initialize available widgets
    widgets_ = {
        // Layout
        {"Container", "div", "[]", "Layout"},
        {"Horizontal Layout", "div", "[=]", "Layout"},
        {"Vertical Layout", "div", "[||]", "Layout"},
        
        // Display
        {"Text", "p", "Aa", "Display"},
        {"Heading", "h1", "H1", "Display"},
        {"Image", "img", "[IMG]", "Display"},
        {"Progress Bar", "progress", "[===]", "Display"},
        
        // Input
        {"Button", "button", "[BTN]", "Input"},
        {"Text Input", "input", "[___]", "Input"},
        {"Checkbox", "checkbox", "[X]", "Input"},
        {"Slider", "range", "[-o-]", "Input"},
        
        // Data
        {"List", "ul", "[*]", "Data"},
        {"Table", "table", "[#]", "Data"},
    };
}

void WidgetPalettePanel::Render(UIEditorContext& ctx) {
    if (!IsVisible()) return;
    
    ImGui::Begin(GetName(), nullptr);
    
    ImGui::Text("Drag widgets to canvas");
    ImGui::Separator();
    
    // Group widgets by category
    std::vector<std::string> categories = {"Layout", "Display", "Input", "Data"};
    
    for (const auto& category : categories) {
        if (ImGui::CollapsingHeader(category.c_str(), ImGuiTreeNodeFlags_DefaultOpen)) {
            RenderCategory(ctx, category);
        }
    }
    
    ImGui::End();
}

void WidgetPalettePanel::RenderCategory(UIEditorContext& ctx, const std::string& category) {
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8, 8));
    
    int buttonsPerRow = 3;
    int buttonIndex = 0;
    
    for (const auto& widget : widgets_) {
        if (widget.category != category) continue;
        
        if (buttonIndex > 0 && buttonIndex % buttonsPerRow != 0) {
            ImGui::SameLine();
        }
        
        ImGui::PushID(widget.name.c_str());
        
        ImVec2 buttonSize(70, 60);
        if (ImGui::Button(widget.icon.c_str(), buttonSize)) {
            // Add widget to canvas on click
            ctx.GetWidgetTree().AddWidget(nullptr, widget.type);
            ctx.GetDocument().SetDirty();
        }
        
        // Drag source for drag-and-drop
        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            const char* type = widget.type.c_str();
            ImGui::SetDragDropPayload("WIDGET_TYPE", type, strlen(type) + 1);
            ImGui::Text("Add %s", widget.name.c_str());
            ImGui::EndDragDropSource();
        }
        
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("%s", widget.name.c_str());
        }
        
        ImGui::PopID();
        buttonIndex++;
    }
    
    ImGui::PopStyleVar();
}

}  // namespace ued
