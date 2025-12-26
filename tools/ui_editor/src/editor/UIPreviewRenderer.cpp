#include "UIPreviewRenderer.h"

#include <sstream>

#include "engine/core/Log.h"
#include "core/UIWidgetTree.h"

namespace ued {

UIPreviewRenderer::UIPreviewRenderer() {}

UIPreviewRenderer::~UIPreviewRenderer() {
    Shutdown();
}
// Static counter to track how many renderers are using RmlUI
static int s_rmlInitCount = 0;

void UIPreviewRenderer::Initialize(int width, int height) {
    if (initialized_) return;
    if (initFailed_) return;  // Don't retry if init already failed
    
    SE_LOG_INFO("UIPreviewRenderer: Initializing with size {}x{}", width, height);
    
    width_ = width;
    height_ = height;
    
    // Create our own RmlUI interfaces
    systemInterface_ = new se::RmlUiSystemInterface();
    renderInterface_ = new se::RmlUiRenderInterface();
    // Note: We don't set FontEngineInterface - RmlUI will use its default FreeType-based engine
    
    // Set the interfaces for RmlUI
    Rml::SetSystemInterface(systemInterface_);
    Rml::SetRenderInterface(renderInterface_);
    
    // Initialize RmlUI only if not already initialized
    if (s_rmlInitCount == 0) {
        if (!Rml::Initialise()) {
            SE_LOG_ERROR("UIPreviewRenderer: Failed to initialize RmlUI");
            initFailed_ = true;
            return;
        }
        
        // Load default font for preview
        if (!Rml::LoadFontFace("assets/fonts/LatoLatin-Regular.ttf")) {
            SE_LOG_WARN("UIPreviewRenderer: Could not load LatoLatin-Regular.ttf, trying system font");
            // Try a Windows system font as fallback
            Rml::LoadFontFace("C:/Windows/Fonts/arial.ttf");
        }
    }
    s_rmlInitCount++;
    rmlInitializedByUs_ = true;
    
    renderInterface_->SetViewport(width, height);
    
    // Create our context for the editor preview
    std::string contextName = "ui_editor_preview_" + std::to_string(s_rmlInitCount);
    context_ = Rml::CreateContext(contextName, Rml::Vector2i(width, height));
    if (!context_) {
        SE_LOG_ERROR("UIPreviewRenderer: Failed to create RmlUi context");
        initFailed_ = true;
        return;
    }
    
    CreateFramebuffer(width, height);
    
    initialized_ = true;
    SE_LOG_INFO("UIPreviewRenderer: Initialized successfully");
}

void UIPreviewRenderer::Shutdown() {
    if (!initialized_ && !rmlInitializedByUs_) return;
    
    SE_LOG_INFO("UIPreviewRenderer: Shutting down");
    
    if (document_) {
        document_->Close();
        document_ = nullptr;
    }
    
    if (context_) {
        Rml::RemoveContext(context_->GetName());
        context_ = nullptr;
    }
    
    // Shutdown RmlUI only if we are the last user
    if (rmlInitializedByUs_) {
        s_rmlInitCount--;
        if (s_rmlInitCount == 0) {
            Rml::Shutdown();
        }
        rmlInitializedByUs_ = false;
    }
    
    if (framebuffer_) glDeleteFramebuffers(1, &framebuffer_);
    if (textureColorbuffer_) glDeleteTextures(1, &textureColorbuffer_);
    if (rbo_) glDeleteRenderbuffers(1, &rbo_);
    
    framebuffer_ = 0;
    textureColorbuffer_ = 0;
    rbo_ = 0;
    
    delete systemInterface_;
    delete renderInterface_;
    systemInterface_ = nullptr;
    renderInterface_ = nullptr;
    
    initialized_ = false;
}

void UIPreviewRenderer::CreateFramebuffer(int width, int height) {
    glGenFramebuffers(1, &framebuffer_);
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    
    glGenTextures(1, &textureColorbuffer_);
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, textureColorbuffer_, 0);
    
    glGenRenderbuffers(1, &rbo_);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo_);
    
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SE_LOG_ERROR("UIPreviewRenderer: Framebuffer is not complete!");
    }
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void UIPreviewRenderer::SetViewport(int width, int height) {
    if (width <= 0 || height <= 0) return;
    if (width == width_ && height == height_) return;
    
    width_ = width;
    height_ = height;
    
    ResizeFramebuffer(width, height);
    
    if (context_) {
        context_->SetDimensions(Rml::Vector2i(width, height));
    }
    if (renderInterface_) {
        renderInterface_->SetViewport(width, height);
    }
}

void UIPreviewRenderer::ResizeFramebuffer(int width, int height) {
    glBindTexture(GL_TEXTURE_2D, textureColorbuffer_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    
    glBindRenderbuffer(GL_RENDERBUFFER, rbo_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
}

void UIPreviewRenderer::UpdateFromWidgetTree(const UIWidgetTree& tree, const std::string& styleContent) {
    if (!initialized_ || !context_) return;
    
    std::string rmlContent = tree.GenerateRml();
    
    // Only update if content changed
    if (rmlContent == lastRmlContent_ && styleContent == lastStyleContent_) {
        return;
    }
    
    lastRmlContent_ = rmlContent;
    lastStyleContent_ = styleContent;
    
    // Close existing document
    if (document_) {
        document_->Close();
        document_ = nullptr;
    }
    
    // Load the new document from string
    document_ = context_->LoadDocumentFromMemory(rmlContent);
    if (document_) {
        document_->Show();
    }
}

void UIPreviewRenderer::Render() {
    if (!initialized_ || !context_) return;
    
    // Bind our framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_);
    glViewport(0, 0, width_, height_);
    
    // Update render interface viewport
    if (renderInterface_) {
        renderInterface_->SetViewport(width_, height_);
    }
    
    // Clear with distinct background to verify texture is working
    glClearColor(0.12f, 0.12f, 0.16f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
    
    // Setup OpenGL state for RmlUI
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_STENCIL_TEST);
    glStencilFunc(GL_ALWAYS, 0, 0xFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    
    // Update and render RmlUI
    context_->Update();
    context_->Render();
    
    // Restore state
    glDisable(GL_STENCIL_TEST);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

}  // namespace ued
