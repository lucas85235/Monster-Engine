#pragma once

#include <string>
#include <vector>

class EditorContext;

struct ClipInfo {
    std::string filename;
    std::string fullPath;
};

class AssetBrowserPanel {
public:
    explicit AssetBrowserPanel(EditorContext& context);
    ~AssetBrowserPanel();

    void Render();
    void RefreshAssets();
    
    const std::string& GetSelectedClipPath() const;

private:
    EditorContext& context_;
    std::vector<ClipInfo> animationClips_;
    int selectedClip_ = -1;
};
