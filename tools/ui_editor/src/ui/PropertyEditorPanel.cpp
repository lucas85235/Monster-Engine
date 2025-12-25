#include "PropertyEditorPanel.h"

#include <set>
#include <string>
#include <cstdio>

#include "core/UIEditorContext.h"

namespace ued {

void PropertyEditorPanel::Render(UIEditorContext& ctx) {
    if (!IsVisible()) return;
    
    ImGui::Begin(GetName(), nullptr);
    
    UIWidgetNode* selected = ctx.GetWidgetTree().GetSelected();
    
    if (!selected) {
        ImGui::TextDisabled("No widget selected");
        ImGui::End();
        return;
    }
    
    ImGui::Text("Widget: %s", selected->type.c_str());
    ImGui::Separator();
    
    if (ImGui::CollapsingHeader("Identity", ImGuiTreeNodeFlags_DefaultOpen)) {
        // ID
        strncpy_s(idBuffer_, selected->id.c_str(), sizeof(idBuffer_) - 1);
        if (ImGui::InputText("ID", idBuffer_, sizeof(idBuffer_))) {
            selected->id = idBuffer_;
            ctx.GetDocument().SetDirty();
        }
        
        // Type (read-only)
        ImGui::Text("Type: %s", selected->type.c_str());
    }
    
    if (ImGui::CollapsingHeader("Layout & Alignment", ImGuiTreeNodeFlags_DefaultOpen)) {
        RenderAlignmentControls(ctx, selected);
    }
    
    if (ImGui::CollapsingHeader("Content", ImGuiTreeNodeFlags_DefaultOpen)) {
        RenderAttributeEditor(ctx, selected);
    }
    
    if (ImGui::CollapsingHeader("Inline Styles", ImGuiTreeNodeFlags_DefaultOpen)) {
        RenderStyleEditor(ctx, selected);
    }
    
    ImGui::End();
}

void PropertyEditorPanel::RenderAttributeEditor(UIEditorContext& ctx, UIWidgetNode* node) {
    // Text content
    strncpy_s(textBuffer_, node->textContent.c_str(), sizeof(textBuffer_) - 1);
    if (ImGui::InputTextMultiline("Text", textBuffer_, sizeof(textBuffer_), ImVec2(-1, 60))) {
        node->textContent = textBuffer_;
        ctx.GetDocument().SetDirty();
    }
    
    // Class attribute
    std::string classValue;
    for (const auto& [key, value] : node->attributes) {
        if (key == "class") {
            classValue = value;
            break;
        }
    }
    strncpy_s(classBuffer_, classValue.c_str(), sizeof(classBuffer_) - 1);
    if (ImGui::InputText("Class", classBuffer_, sizeof(classBuffer_))) {
        bool found = false;
        for (auto& [key, value] : node->attributes) {
            if (key == "class") {
                value = classBuffer_;
                found = true;
                break;
            }
        }
        if (!found && strlen(classBuffer_) > 0) {
            node->attributes.push_back({"class", classBuffer_});
        }
        ctx.GetDocument().SetDirty();
    }
    
    ImGui::Separator();
    ImGui::Text("Attributes:");
    
    // Existing attributes
    int toDelete = -1;
    for (int i = 0; i < (int)node->attributes.size(); i++) {
        auto& [key, value] = node->attributes[i];
        if (key == "class") continue; // Already shown above
        
        ImGui::PushID(i);
        
        char keyBuf[64], valBuf[256];
        strncpy_s(keyBuf, key.c_str(), sizeof(keyBuf) - 1);
        strncpy_s(valBuf, value.c_str(), sizeof(valBuf) - 1);
        
        ImGui::SetNextItemWidth(80);
        if (ImGui::InputText("##key", keyBuf, sizeof(keyBuf))) {
            key = keyBuf;
            ctx.GetDocument().SetDirty();
        }
        ImGui::SameLine();
        ImGui::SetNextItemWidth(120);
        if (ImGui::InputText("##val", valBuf, sizeof(valBuf))) {
            value = valBuf;
            ctx.GetDocument().SetDirty();
        }
        ImGui::SameLine();
        if (ImGui::SmallButton("X")) {
            toDelete = i;
        }
        
        ImGui::PopID();
    }
    
    if (toDelete >= 0) {
        node->attributes.erase(node->attributes.begin() + toDelete);
        ctx.GetDocument().SetDirty();
    }
    
    if (ImGui::SmallButton("+ Add Attribute")) {
        node->attributes.push_back({"", ""});
        ctx.GetDocument().SetDirty();
    }
}

// Helper to check if property is a color
static bool IsColorProperty_(const std::string& prop) {
    return prop.find("color") != std::string::npos || 
           prop.find("background") != std::string::npos;
}

// Helper to check if property needs px units
static bool IsSizeProperty_(const std::string& prop) {
    static const std::set<std::string> sizePropSet = {
        "width", "height", "min-width", "min-height", "max-width", "max-height",
        "margin", "margin-top", "margin-right", "margin-bottom", "margin-left",
        "padding", "padding-top", "padding-right", "padding-bottom", "padding-left",
        "border", "border-width", "border-radius", "font-size", "line-height",
        "top", "right", "bottom", "left"
    };
    return sizePropSet.find(prop) != sizePropSet.end();
}

// Parse hex color to float array
static bool ParseHexColor(const std::string& hex, float* rgba) {
    if (hex.empty() || hex[0] != '#') return false;
    std::string h = hex.substr(1);
    if (h.length() == 6) h += "ff";  // Add alpha
    if (h.length() != 8) return false;
    
    unsigned int r, g, b, a;
    if (sscanf_s(h.c_str(), "%02x%02x%02x%02x", &r, &g, &b, &a) == 4) {
        rgba[0] = r / 255.0f;
        rgba[1] = g / 255.0f;
        rgba[2] = b / 255.0f;
        rgba[3] = a / 255.0f;
        return true;
    }
    return false;
}

// Convert float array to hex color
static std::string FloatToHexColor(const float* rgba) {
    char buf[10];
    snprintf(buf, sizeof(buf), "#%02x%02x%02x",
        (int)(rgba[0] * 255), (int)(rgba[1] * 255), (int)(rgba[2] * 255));
    return buf;
}

void PropertyEditorPanel::RenderStyleEditor(UIEditorContext& ctx, UIWidgetNode* node) {
    // Common style properties organized by category
    static const char* sizeProps[] = { "width", "height", "min-width", "min-height", "margin", "padding" };
    static const char* colorProps[] = { "background-color", "color", "border-color" };
    static const char* otherProps[] = { "font-size", "border-radius", "border-width", "display" };
    
    // Existing inline styles
    int toDelete = -1;
    for (int i = 0; i < (int)node->styles.size(); i++) {
        auto& [prop, value] = node->styles[i];
        
        ImGui::PushID(1000 + i);
        
        // Property name (read-only label)
        ImGui::Text("%s", prop.c_str());
        ImGui::SameLine();
        
        // Different editors based on property type
        if (IsColorProperty_(prop)) {
            // Color picker
            float col[4] = {0.5f, 0.5f, 0.5f, 1.0f};
            ParseHexColor(value, col);
            
            ImGui::SetNextItemWidth(150);
            if (ImGui::ColorEdit3("##color", col, ImGuiColorEditFlags_NoInputs | ImGuiColorEditFlags_NoLabel)) {
                value = FloatToHexColor(col);
                ctx.GetDocument().SetDirty();
            }
            ImGui::SameLine();
            ImGui::Text("%s", value.c_str());
        } else if (IsSizeProperty_(prop)) {
            // Numeric input with px suffix
            float numVal = 0;
            std::string strVal = value;
            // Remove px suffix if present
            if (strVal.length() > 2 && strVal.substr(strVal.length()-2) == "px") {
                strVal = strVal.substr(0, strVal.length()-2);
            }
            try { numVal = std::stof(strVal); } catch(...) {}
            
            ImGui::SetNextItemWidth(80);
            if (ImGui::DragFloat("##size", &numVal, 1.0f, 0.0f, 2000.0f, "%.0f")) {
                value = std::to_string((int)numVal);  // Store without px, formatter will add it
                ctx.GetDocument().SetDirty();
            }
            ImGui::SameLine();
            ImGui::TextDisabled("px");
        } else if (prop == "display") {
            // Dropdown for display
            static const char* displayOpts[] = { "block", "inline", "inline-block", "flex", "none" };
            int current = 0;
            for (int j = 0; j < 5; j++) {
                if (value == displayOpts[j]) { current = j; break; }
            }
            ImGui::SetNextItemWidth(100);
            if (ImGui::Combo("##display", &current, displayOpts, 5)) {
                value = displayOpts[current];
                ctx.GetDocument().SetDirty();
            }
        } else {
            // Default text input
            char valBuf[128];
            strncpy_s(valBuf, value.c_str(), sizeof(valBuf) - 1);
            ImGui::SetNextItemWidth(100);
            if (ImGui::InputText("##val", valBuf, sizeof(valBuf))) {
                value = valBuf;
                ctx.GetDocument().SetDirty();
            }
        }
        
        ImGui::SameLine();
        if (ImGui::SmallButton("X")) {
            toDelete = i;
        }
        
        ImGui::PopID();
    }
    
    if (toDelete >= 0) {
        node->styles.erase(node->styles.begin() + toDelete);
        ctx.GetDocument().SetDirty();
    }
    
    // Add style button with popup
    if (ImGui::Button("+ Add Style")) {
        ImGui::OpenPopup("add_style_popup");
    }
    
    if (ImGui::BeginPopup("add_style_popup")) {
        if (ImGui::BeginMenu("Size")) {
            for (const char* prop : sizeProps) {
                if (ImGui::MenuItem(prop)) {
                    node->styles.push_back({prop, "100"});
                    ctx.GetDocument().SetDirty();
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Color")) {
            for (const char* prop : colorProps) {
                if (ImGui::MenuItem(prop)) {
                    node->styles.push_back({prop, "#ffffff"});
                    ctx.GetDocument().SetDirty();
                }
            }
            ImGui::EndMenu();
        }
        if (ImGui::BeginMenu("Other")) {
            for (const char* prop : otherProps) {
                if (ImGui::MenuItem(prop)) {
                    std::string defaultVal = (prop == std::string("display")) ? "block" : "0";
                    node->styles.push_back({prop, defaultVal});
                    ctx.GetDocument().SetDirty();
                }
            }
            ImGui::EndMenu();
        }
        ImGui::EndPopup();
    }
}

// Helper to set or update a style property
static void SetStyleProperty(UIWidgetNode* node, const std::string& prop, const std::string& value) {
    for (auto& [key, val] : node->styles) {
        if (key == prop) {
            val = value;
            return;
        }
    }
    node->styles.push_back({prop, value});
}

void PropertyEditorPanel::RenderAlignmentControls(UIEditorContext& ctx, UIWidgetNode* node) {
    ImGui::Text("Horizontal Alignment:");
    
    if (ImGui::Button("Left", ImVec2(50, 0))) {
        SetStyleProperty(node, "margin-left", "0");
        SetStyleProperty(node, "margin-right", "auto");
        ctx.GetDocument().SetDirty();
    }
    ImGui::SameLine();
    
    if (ImGui::Button("Center", ImVec2(50, 0))) {
        SetStyleProperty(node, "margin-left", "auto");
        SetStyleProperty(node, "margin-right", "auto");
        ctx.GetDocument().SetDirty();
    }
    ImGui::SameLine();
    
    if (ImGui::Button("Right", ImVec2(50, 0))) {
        SetStyleProperty(node, "margin-left", "auto");
        SetStyleProperty(node, "margin-right", "0");
        ctx.GetDocument().SetDirty();
    }
    ImGui::SameLine();
    
    if (ImGui::Button("Stretch", ImVec2(50, 0))) {
        SetStyleProperty(node, "width", "100%");
        SetStyleProperty(node, "margin-left", "0");
        SetStyleProperty(node, "margin-right", "0");
        ctx.GetDocument().SetDirty();
    }
    
    ImGui::Spacing();
    ImGui::Text("Text Alignment:");
    
    static const char* textAlignOpts[] = { "left", "center", "right", "justify" };
    std::string currentTextAlign = "left";
    for (const auto& [key, val] : node->styles) {
        if (key == "text-align") {
            currentTextAlign = val;
            break;
        }
    }
    int textAlignIdx = 0;
    for (int i = 0; i < 4; i++) {
        if (currentTextAlign == textAlignOpts[i]) {
            textAlignIdx = i;
            break;
        }
    }
    
    ImGui::SetNextItemWidth(120);
    if (ImGui::Combo("##textalign", &textAlignIdx, textAlignOpts, 4)) {
        SetStyleProperty(node, "text-align", textAlignOpts[textAlignIdx]);
        ctx.GetDocument().SetDirty();
    }
}

}  // namespace ued

