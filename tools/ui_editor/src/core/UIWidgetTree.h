#pragma once

#include <string>
#include <vector>
#include <memory>

#include "Engine.h"

namespace ued {

struct UIWidgetNode {
    std::string id;
    std::string type;        // e.g., "div", "button", "p", "input"
    std::string textContent;
    
    std::vector<std::pair<std::string, std::string>> attributes;
    std::vector<std::pair<std::string, std::string>> styles;
    
    std::vector<se::Scope<UIWidgetNode>> children;
    UIWidgetNode* parent = nullptr;
    
    bool isSelected = false;
};

class UIWidgetTree {
public:
    UIWidgetTree();
    
    UIWidgetNode* GetRoot() { return root_.get(); }
    const UIWidgetNode* GetRoot() const { return root_.get(); }
    
    UIWidgetNode* GetSelected() { return selected_; }
    void SetSelected(UIWidgetNode* node) { selected_ = node; }
    
    UIWidgetNode* AddWidget(UIWidgetNode* parent, const std::string& type);
    void RemoveWidget(UIWidgetNode* node);
    void MoveWidget(UIWidgetNode* node, UIWidgetNode* newParent, int index = -1);
    
    void Clear();
    
    std::string GenerateRml() const;

private:
    std::string GenerateRmlRecursive(const UIWidgetNode* node, int indent) const;
    int nextId_ = 1;
    
    se::Scope<UIWidgetNode> root_;
    UIWidgetNode* selected_ = nullptr;
};

}  // namespace ued
