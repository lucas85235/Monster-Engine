#pragma once

#include <string>

#include "Engine.h"
#include "UIWidgetTree.h"

namespace ued {

class UIDocument {
public:
    UIDocument();
    
    void New();
    bool Load(const std::string& filepath);
    bool Save();
    bool SaveAs(const std::string& filepath);
    
    void Update();
    
    bool IsDirty() const { return dirty_; }
    void SetDirty(bool dirty = true) { dirty_ = dirty; }
    
    const std::string& GetFilePath() const { return filepath_; }
    const std::string& GetTitle() const { return title_; }
    
    UIWidgetTree& GetWidgetTree() { return widgetTree_; }
    const UIWidgetTree& GetWidgetTree() const { return widgetTree_; }
    
    std::string& GetStyleContent() { return styleContent_; }
    const std::string& GetStyleContent() const { return styleContent_; }

private:
    std::string filepath_;
    std::string title_ = "Untitled";
    bool dirty_ = false;
    
    UIWidgetTree widgetTree_;
    std::string styleContent_;
};

}  // namespace ued
