#include "editor/EditorFramebuffer.h"

#include <filament/Engine.h>
#include <filament/RenderTarget.h>
#include <filament/Texture.h>
#include <filament/View.h>
#include <filament/Scene.h>
#include <filament/Camera.h>
#include <filament/Viewport.h>
#include <utils/EntityManager.h>

#include "engine/Application.h"
#include "engine/Log.h"

namespace mst {

EditorFramebuffer::EditorFramebuffer(uint32_t width, uint32_t height)
    : width_(width), height_(height) {
    engine_ = se::Application::Get().GetFilamentContext().GetEngine();
    if (!engine_) {
        SE_LOG_ERROR("EditorFramebuffer: Filament engine not available!");
        return;
    }
    CreateResources();
    SE_LOG_INFO("EditorFramebuffer: Created {}x{} offscreen target", width, height);
}

EditorFramebuffer::~EditorFramebuffer() {
    DestroyResources();
}

void EditorFramebuffer::CreateResources() {
    if (!engine_) return;

    // Color texture: SAMPLEABLE so ImGui can read it, COLOR_ATTACHMENT so Filament can render to it
    colorTexture_ = filament::Texture::Builder()
        .width(width_)
        .height(height_)
        .levels(1)
        .usage(filament::Texture::Usage::COLOR_ATTACHMENT | filament::Texture::Usage::SAMPLEABLE)
        .format(filament::Texture::InternalFormat::RGBA8)
        .build(*engine_);

    // Depth texture for proper depth testing
    depthTexture_ = filament::Texture::Builder()
        .width(width_)
        .height(height_)
        .levels(1)
        .usage(filament::Texture::Usage::DEPTH_ATTACHMENT)
        .format(filament::Texture::InternalFormat::DEPTH24)
        .build(*engine_);

    // RenderTarget combining color + depth
    renderTarget_ = filament::RenderTarget::Builder()
        .texture(filament::RenderTarget::AttachmentPoint::COLOR, colorTexture_)
        .texture(filament::RenderTarget::AttachmentPoint::DEPTH, depthTexture_)
        .build(*engine_);

    // Create a camera entity for the offscreen view
    cameraEntity_ = utils::EntityManager::get().create();
    camera_ = engine_->createCamera(cameraEntity_);

    // Create a dedicated View for the offscreen scene
    view_ = engine_->createView();
    view_->setRenderTarget(renderTarget_);
    view_->setViewport({0, 0, width_, height_});
    view_->setCamera(camera_);
    view_->setName("EditorViewport");

    // Configure offscreen view settings
    view_->setPostProcessingEnabled(false);  // Avoid issues with small RTs
    view_->setShadowingEnabled(true);
    view_->setScreenSpaceRefractionEnabled(false);
}

void EditorFramebuffer::DestroyResources() {
    if (!engine_) return;

    if (view_) {
        engine_->destroy(view_);
        view_ = nullptr;
    }
    if (camera_) {
        engine_->destroyCameraComponent(cameraEntity_);
        camera_ = nullptr;
    }
    if (cameraEntity_) {
        utils::EntityManager::get().destroy(cameraEntity_);
        cameraEntity_ = {};
    }
    if (renderTarget_) {
        engine_->destroy(renderTarget_);
        renderTarget_ = nullptr;
    }
    if (depthTexture_) {
        engine_->destroy(depthTexture_);
        depthTexture_ = nullptr;
    }
    if (colorTexture_) {
        engine_->destroy(colorTexture_);
        colorTexture_ = nullptr;
    }
}

void EditorFramebuffer::Resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0) return;
    if (width == width_ && height == height_) return;

    width_ = width;
    height_ = height;

    // Recreate all GPU resources at the new size
    DestroyResources();
    CreateResources();
}

uintptr_t EditorFramebuffer::GetColorAttachmentAsImTextureID() const {
    return reinterpret_cast<uintptr_t>(colorTexture_);
}

void EditorFramebuffer::SetScene(filament::Scene* scene) {
    if (view_ && scene) {
        view_->setScene(scene);
    }
}

}  // namespace mst
