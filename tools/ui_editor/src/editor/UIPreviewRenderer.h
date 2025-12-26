#pragma once

#include <RmlUi/Core.h>
#include <glad/glad.h>

#include "Engine.h"
#include "engine/ui/RmlUiInterfaces.h"

namespace ued {

class UIWidgetTree;

class UIPreviewRenderer {
public:
    UIPreviewRenderer();
    ~UIPreviewRenderer();
    
    void Initialize(int width, int height);
    void Shutdown();
    
    void SetViewport(int width, int height);
    void UpdateFromWidgetTree(const UIWidgetTree& tree, const std::string& styleContent);
    void Render();
    
    GLuint GetTextureId() const { return textureColorbuffer_; }
    
    bool IsInitialized() const { return initialized_; }

private:
    void CreateFramebuffer(int width, int height);
    void ResizeFramebuffer(int width, int height);
    
    bool initialized_ = false;
    bool initFailed_ = false;
    bool rmlInitializedByUs_ = false;
    
    Rml::Context* context_ = nullptr;
    Rml::ElementDocument* document_ = nullptr;
    
    se::RmlUiSystemInterface* systemInterface_ = nullptr;
    se::RmlUiRenderInterface* renderInterface_ = nullptr;
    
    GLuint framebuffer_ = 0;
    GLuint textureColorbuffer_ = 0;
    GLuint rbo_ = 0;
    
    int width_ = 0;
    int height_ = 0;
    
    std::string lastRmlContent_;
    std::string lastStyleContent_;
};

}  // namespace ued
