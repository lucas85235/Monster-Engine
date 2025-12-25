#pragma once
/**
 * FileDialogManager.h - Modal file dialog management.
 *
 * Handles export and open dialogs, providing a clean interface
 * for file operations without cluttering the main layer.
 */

#include <string>
#include <functional>

namespace mst {

class EditorContext;

struct FileDialogResult {
    bool confirmed = false;
    std::string filename;
};

class FileDialogManager {
public:
    using DialogCallback = std::function<void(const FileDialogResult&)>;
    
    void ShowExportDialog(DialogCallback callback);
    void ShowOpenDialog(DialogCallback callback);
    
    void Render(EditorContext& ctx);
    
    bool IsDialogOpen() const { return showExport_ || showOpen_; }

private:
    void RenderExportDialog();
    void RenderOpenDialog();
    
    bool showExport_ = false;
    bool showOpen_ = false;
    
    char exportFileName_[256] = "untitled";
    char openFileName_[256] = "";
    
    DialogCallback exportCallback_;
    DialogCallback openCallback_;
};

}  // namespace mst
