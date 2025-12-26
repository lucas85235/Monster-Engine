#include "UIDocument.h"

#include <fstream>
#include <sstream>

#include "engine/Log.h"

namespace ued {

UIDocument::UIDocument() {
    New();
}

void UIDocument::New() {
    filepath_.clear();
    title_ = "Untitled";
    dirty_ = false;
    widgetTree_.Clear();
    styleContent_ = R"(/* Default styles */
body {
    font-family: LatoLatin;
    font-size: 16px;
    color: #ffffff;
}

.button {
    background-color: #4a90d9;
    padding: 10px 20px;
    border-radius: 4px;
}

.button:hover {
    background-color: #5a9fe9;
}
)";
    
    SE_LOG_INFO("Created new UI document");
}

bool UIDocument::Load(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) {
        SE_LOG_ERROR("Failed to open file: {}", filepath);
        return false;
    }
    
    // TODO: Parse RML file and populate widget tree
    filepath_ = filepath;
    
    size_t lastSlash = filepath.find_last_of("/\\");
    title_ = (lastSlash != std::string::npos) ? filepath.substr(lastSlash + 1) : filepath;
    
    dirty_ = false;
    SE_LOG_INFO("Loaded UI document: {}", filepath);
    return true;
}

bool UIDocument::Save() {
    if (filepath_.empty()) {
        SE_LOG_WARN("No filepath set, cannot save");
        return false;
    }
    return SaveAs(filepath_);
}

bool UIDocument::SaveAs(const std::string& filepath) {
    std::ofstream rmlFile(filepath);
    if (!rmlFile.is_open()) {
        SE_LOG_ERROR("Failed to create file: {}", filepath);
        return false;
    }
    
    rmlFile << widgetTree_.GenerateRml();
    rmlFile.close();
    
    // Save RCSS file alongside
    std::string rcssPath = filepath;
    size_t dotPos = rcssPath.rfind('.');
    if (dotPos != std::string::npos) {
        rcssPath = rcssPath.substr(0, dotPos) + ".rcss";
    } else {
        rcssPath += ".rcss";
    }
    
    std::ofstream rcssFile(rcssPath);
    if (rcssFile.is_open()) {
        rcssFile << styleContent_;
        rcssFile.close();
    }
    
    filepath_ = filepath;
    size_t lastSlash = filepath.find_last_of("/\\");
    title_ = (lastSlash != std::string::npos) ? filepath.substr(lastSlash + 1) : filepath;
    dirty_ = false;
    
    SE_LOG_INFO("Saved UI document: {}", filepath);
    return true;
}

void UIDocument::Update() {
    // Placeholder for any per-frame document updates
}

}  // namespace ued
