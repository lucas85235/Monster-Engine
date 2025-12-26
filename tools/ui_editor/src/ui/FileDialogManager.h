#pragma once

#include <string>
#include <functional>

namespace ued {

class UIEditorContext;

struct FileDialogResult {
    bool confirmed = false;
    std::string filename;
};

class FileDialogManager {
public:
    using DialogCallback = std::function<void(const FileDialogResult&)>;
    
    void ShowSaveDialog(DialogCallback callback);
    void ShowOpenDialog(DialogCallback callback);
    
    void Render();
    
    bool IsDialogOpen() const { return showSave_ || showOpen_; }

private:
    void RenderSaveDialog();
    void RenderOpenDialog();
    
    bool showSave_ = false;
    bool showOpen_ = false;
    
    char saveFileName_[256] = "untitled";
    char openFileName_[256] = "";
    
    DialogCallback saveCallback_;
    DialogCallback openCallback_;
};

}  // namespace ued
