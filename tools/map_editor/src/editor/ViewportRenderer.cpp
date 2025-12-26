#include "ViewportRenderer.h"

#include <glad/glad.h>

#include "engine/core/Application.h"
#include "engine/core/Log.h"

using namespace se;

namespace mst {

ViewportRenderer::ViewportRenderer(uint32_t width, uint32_t height)
    : width_(width), height_(height) {
    framebuffer_ = se::CreateScope<EditorFramebuffer>(width, height);
    grid_ = se::CreateScope<EditorGrid>();
    colliderDebug_ = se::CreateScope<ColliderDebugRenderer>();
    
    SE_LOG_INFO("ViewportRenderer: Initialized {}x{}", width, height);
}

void ViewportRenderer::BeginFrame() {
    if (framebuffer_) {
        framebuffer_->Bind();
        
        glClearColor(settings_.clearColor.x, settings_.clearColor.y, settings_.clearColor.z, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}

void ViewportRenderer::RenderScene(const EditorCamera& camera, se::Scene& scene) {
    float aspectRatio = GetAspectRatio();
    scene.OnRender(camera.GetCamera(), aspectRatio);
}

void ViewportRenderer::RenderGrid(const EditorCamera& camera) {
    if (!settings_.showGrid || !grid_) return;
    
    float aspectRatio = GetAspectRatio();
    se::Matrix4 view = camera.GetCamera().getViewMatrix();
    se::Matrix4 projection = camera.GetCamera().getProjectionMatrix(aspectRatio);
    
    grid_->Render(view, projection);
}

void ViewportRenderer::RenderColliderDebug(const EditorCamera& camera, se::Scene& scene) {
    if (!settings_.showColliderDebug || !colliderDebug_) return;
    
    float aspectRatio = GetAspectRatio();
    se::Matrix4 view = camera.GetCamera().getViewMatrix();
    se::Matrix4 projection = camera.GetCamera().getProjectionMatrix(aspectRatio);
    
    colliderDebug_->Render(view, projection, scene);
}

void ViewportRenderer::EndFrame() {
    if (framebuffer_) {
        framebuffer_->Unbind();
    }
    
    auto& window = se::Application::Get().GetWindow();
    glViewport(0, 0, window.GetWidth(), window.GetHeight());
}

void ViewportRenderer::Resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return;
    if (width == width_ && height == height_) return;
    
    width_ = width;
    height_ = height;
    
    if (framebuffer_) {
        framebuffer_->Resize(width, height);
    }
}

uint32_t ViewportRenderer::GetColorAttachment() const {
    return framebuffer_ ? framebuffer_->GetColorAttachment() : 0;
}

float ViewportRenderer::GetAspectRatio() const {
    if (height_ == 0) return 1.0f;
    return static_cast<float>(width_) / static_cast<float>(height_);
}

}  // namespace mst
