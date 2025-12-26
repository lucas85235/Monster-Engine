#include "UIWidgetTree.h"

#include <sstream>
#include <algorithm>
#include <set>
#include <cctype>

#include "engine/core/Log.h"

namespace ued {

UIWidgetTree::UIWidgetTree() {
    root_ = se::CreateScope<UIWidgetNode>();
    root_->id = "root";
    root_->type = "body";
}

UIWidgetNode* UIWidgetTree::AddWidget(UIWidgetNode* parent, const std::string& type) {
    if (!parent) parent = root_.get();
    
    auto node = se::CreateScope<UIWidgetNode>();
    node->id = "widget_" + std::to_string(nextId_++);
    node->type = type;
    node->parent = parent;
    
    UIWidgetNode* rawPtr = node.get();
    parent->children.push_back(std::move(node));
    
    SE_LOG_INFO("Added widget '{}' of type '{}' to parent '{}'", rawPtr->id, type, parent->id);
    return rawPtr;
}

void UIWidgetTree::RemoveWidget(UIWidgetNode* node) {
    if (!node || !node->parent) return;
    
    auto& siblings = node->parent->children;
    auto it = std::find_if(siblings.begin(), siblings.end(),
        [node](const se::Scope<UIWidgetNode>& ptr) { return ptr.get() == node; });
    
    if (it != siblings.end()) {
        if (selected_ == node) selected_ = nullptr;
        SE_LOG_INFO("Removed widget '{}'", node->id);
        siblings.erase(it);
    }
}

void UIWidgetTree::MoveWidget(UIWidgetNode* node, UIWidgetNode* newParent, int index) {
    if (!node || !node->parent || !newParent) return;
    if (node == newParent) return;
    
    auto& oldSiblings = node->parent->children;
    auto it = std::find_if(oldSiblings.begin(), oldSiblings.end(),
        [node](const se::Scope<UIWidgetNode>& ptr) { return ptr.get() == node; });
    
    if (it != oldSiblings.end()) {
        se::Scope<UIWidgetNode> nodePtr = std::move(*it);
        oldSiblings.erase(it);
        
        nodePtr->parent = newParent;
        
        if (index < 0 || index >= static_cast<int>(newParent->children.size())) {
            newParent->children.push_back(std::move(nodePtr));
        } else {
            newParent->children.insert(newParent->children.begin() + index, std::move(nodePtr));
        }
        
        SE_LOG_INFO("Moved widget '{}' to new parent '{}'", node->id, newParent->id);
    }
}

void UIWidgetTree::Clear() {
    root_->children.clear();
    selected_ = nullptr;
    nextId_ = 1;
    SE_LOG_INFO("Widget tree cleared");
}

std::string UIWidgetTree::GenerateRml() const {
    std::ostringstream ss;
    ss << "<rml>\n";
    ss << "<head>\n";
    ss << "  <title>UI Preview</title>\n";
    ss << "  <style>\n";
    ss << "    body { display: block; width: 100%; height: 100%; background-color: #1e1e24; font-family: LatoLatin; font-size: 14px; }\n";
    ss << "    * { font-family: LatoLatin; }\n";
    ss << "    div { display: block; min-width: 50px; min-height: 30px; background-color: #2d2d35; padding: 8px; margin: 4px; border-width: 1px; border-color: #505060; }\n";
    ss << "    button { display: inline-block; min-width: 80px; min-height: 32px; background-color: #4a90d9; padding: 8px 16px; color: #ffffff; border-width: 0px; }\n";
    ss << "    button:hover { background-color: #5aa0e9; }\n";
    ss << "    h1, h2, h3, p, span { display: block; color: #eeeeee; min-height: 20px; }\n";
    ss << "    h1 { font-size: 24px; min-height: 30px; }\n";
    ss << "    h2 { font-size: 20px; min-height: 26px; }\n";
    ss << "    p { font-size: 14px; min-height: 18px; }\n";
    ss << "    input { display: block; min-width: 100px; min-height: 28px; background-color: #333340; border-width: 1px; border-color: #555565; padding: 6px; color: #ffffff; }\n";
    ss << "    input.range { min-width: 150px; min-height: 20px; background-color: #333340; }\n";
    ss << "    input.checkbox { min-width: 24px; min-height: 24px; max-width: 24px; max-height: 24px; }\n";
    ss << "    progress { display: block; min-width: 120px; min-height: 16px; background-color: #333340; }\n";
    ss << "    ul, ol { display: block; min-height: 40px; background-color: #252530; padding: 8px; margin: 4px; }\n";
    ss << "    li { display: block; min-height: 24px; padding: 4px; }\n";
    ss << "    img { width: 64px; height: 64px; background-color: #3a3a45; }\n";
    ss << "  </style>\n";
    ss << "</head>\n";
    ss << GenerateRmlRecursive(root_.get(), 0);
    ss << "</rml>\n";
    return ss.str();
}

// Helper to check if string is a pure number (possibly with decimal)
static bool IsNumericValue(const std::string& val) {
    if (val.empty()) return false;
    bool hasDot = false;
    for (size_t i = 0; i < val.size(); i++) {
        char c = val[i];
        if (c == '.') {
            if (hasDot) return false;
            hasDot = true;
        } else if (!std::isdigit(c)) {
            return false;
        }
    }
    return true;
}

// Properties that need px units when given numeric values
static bool NeedsPxUnit(const std::string& prop) {
    static const std::set<std::string> pxProps = {
        "width", "height", "min-width", "min-height", "max-width", "max-height",
        "margin", "margin-top", "margin-right", "margin-bottom", "margin-left",
        "padding", "padding-top", "padding-right", "padding-bottom", "padding-left",
        "border", "border-width", "border-top-width", "border-right-width", "border-bottom-width", "border-left-width",
        "border-radius", "font-size", "line-height", "top", "right", "bottom", "left"
    };
    return pxProps.find(prop) != pxProps.end();
}

// Format CSS value (add px if needed)
static std::string FormatCssValue(const std::string& prop, const std::string& val) {
    if (val.empty()) return val;
    
    // Skip if already has a unit or is a color/keyword
    if (val.find("px") != std::string::npos ||
        val.find("%") != std::string::npos ||
        val.find("em") != std::string::npos ||
        val.find("rem") != std::string::npos ||
        val.find("#") != std::string::npos ||
        val.find("rgb") != std::string::npos ||
        !IsNumericValue(val)) {
        return val;
    }
    
    // Add px to numeric values for properties that need it
    if (NeedsPxUnit(prop)) {
        return val + "px";
    }
    
    return val;
}

std::string UIWidgetTree::GenerateRmlRecursive(const UIWidgetNode* node, int indent) const {
    std::ostringstream ss;
    std::string indentStr(indent * 2, ' ');
    
    // Handle special types that map to <input type="...">
    std::string tagName = node->type;
    std::string typeAttr;
    std::string classAttr;
    
    if (node->type == "range") {
        tagName = "input";
        typeAttr = "range";
        classAttr = "range";
    } else if (node->type == "checkbox") {
        tagName = "input";
        typeAttr = "checkbox";
        classAttr = "checkbox";
    }
    
    ss << indentStr << "<" << tagName;
    
    if (!node->id.empty() && node->id != "root") {
        ss << " id=\"" << node->id << "\"";
    }
    
    if (!typeAttr.empty()) {
        ss << " type=\"" << typeAttr << "\"";
    }
    
    if (!classAttr.empty()) {
        ss << " class=\"" << classAttr << "\"";
    }
    
    for (const auto& [key, value] : node->attributes) {
        ss << " " << key << "=\"" << value << "\"";
    }
    
    std::string styleStr;
    for (const auto& [prop, val] : node->styles) {
        if (val.empty()) continue;  // Skip empty values
        if (!styleStr.empty()) styleStr += " ";
        styleStr += prop + ": " + FormatCssValue(prop, val) + ";";
    }
    if (!styleStr.empty()) {
        ss << " style=\"" << styleStr << "\"";
    }
    
    if (node->children.empty() && node->textContent.empty()) {
        ss << "/>\n";
    } else {
        ss << ">";
        
        if (!node->textContent.empty()) {
            ss << node->textContent;
        }
        
        if (!node->children.empty()) {
            ss << "\n";
            for (const auto& child : node->children) {
                ss << GenerateRmlRecursive(child.get(), indent + 1);
            }
            ss << indentStr;
        }
        
        ss << "</" << tagName << ">\n";
    }
    
    return ss.str();
}

}  // namespace ued
