#pragma once
/**
 * FileDialogManager.h - ImGui-based file browser dialog.
 *
 * Full file browser with directory navigation, file listing,
 * and path selection - all within ImGui.
 */

#include <string>
#include <functional>
#include <vector>
#include <filesystem>

namespace mst {

class EditorContext;

struct FileDialogResult {
    bool confirmed = false;
    std::string filename;
    std::string fullPath;
};

class FileDialogManager {
public:
    using DialogCallback = std::function<void(const FileDialogResult&)>;
    
    FileDialogManager();
    
    void ShowSaveDialog(DialogCallback callback);
    void ShowOpenDialog(DialogCallback callback);
    
    void Render(EditorContext& ctx);
    
    bool IsDialogOpen() const { return showSave_ || showOpen_; }

private:
    void RenderSaveDialog();
    void RenderOpenDialog();
    void RenderFileBrowser(bool isSaveMode);
    void RefreshDirectory();
    void NavigateTo(const std::filesystem::path& path);
    
    bool showSave_ = false;
    bool showOpen_ = false;
    
    char fileName_[256] = "untitled";
    std::filesystem::path currentPath_;
    
    struct FileEntry {
        std::string name;
        bool isDirectory;
        uintmax_t size;
    };
    std::vector<FileEntry> entries_;
    int selectedIndex_ = -1;
    
    DialogCallback saveCallback_;
    DialogCallback openCallback_;
    
    std::string fileExtension_ = ".mstmap";
};

}  // namespace mst
