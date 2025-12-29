#include "StyleEditorPanel.h"

#include <cstring>

#include "core/UIEditorContext.h"

namespace ued {

void StyleEditorPanel::Render(UIEditorContext& ctx) {
    if (!IsVisible()) return;
    
    ImGui::Begin(GetName(), nullptr);
    
    ImGui::Text("RCSS Stylesheet");
    ImGui::Separator();
    
    std::string& styleContent = ctx.GetDocument().GetStyleContent();
    
    // Resize buffer if needed
    static std::vector<char> buffer;
    if (buffer.size() < styleContent.size() + 4096) {
        buffer.resize(styleContent.size() + 4096);
    }
    std::strncpy(buffer.data(), styleContent.c_str(), buffer.size() - 1);
    buffer[buffer.size() - 1] = '\0';
    
    ImVec2 availSize = ImGui::GetContentRegionAvail();
    availSize.y -= 30; // Leave room for buttons
    
    ImGuiInputTextFlags flags = ImGuiInputTextFlags_AllowTabInput;
    
    if (ImGui::InputTextMultiline("##rcss", buffer.data(), buffer.size(), availSize, flags)) {
        styleContent = buffer.data();
        ctx.GetDocument().SetDirty();
    }
    
    if (ImGui::Button("Format")) {
        // TODO: Basic RCSS formatting
    }
    ImGui::SameLine();
    if (ImGui::Button("Validate")) {
        // TODO: Basic RCSS validation
    }
    
    ImGui::End();
}

}  // namespace ued
