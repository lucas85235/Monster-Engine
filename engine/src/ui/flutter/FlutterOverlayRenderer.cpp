#include "engine/ui/flutter/FlutterOverlayRenderer.h"

#include <algorithm>
#include <cstring>

#include <filament/Camera.h>
#include <filament/Engine.h>
#include <filament/IndexBuffer.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>
#include <filament/RenderableManager.h>
#include <filament/Scene.h>
#include <filament/Texture.h>
#include <filament/TextureSampler.h>
#include <filament/TransformManager.h>
#include <filament/VertexBuffer.h>
#include <filament/View.h>
#include <filament/Viewport.h>

#include <utils/Entity.h>
#include <utils/EntityManager.h>

#include "engine/Log.h"

namespace se::ui::flutter {

// ── Constants ─────────────────────────────────────────────────
static constexpr uint8_t kFlutterLayerMask = 0x20;  // Layer 5

// ── Initialise / Shutdown ─────────────────────────────────────

void FlutterOverlayRenderer::Init(filament::Engine* engine,
                                  filament::View* mainView,
                                  uint32_t width,
                                  uint32_t height) {
    engine_  = engine;
    width_   = width;
    height_  = height;

    // Create dedicated Scene / View / Camera
    scene_  = engine_->createScene();
    view_   = engine_->createView();
    camera_ = engine_->createCamera(utils::EntityManager::get().create());

    view_->setScene(scene_);
    view_->setCamera(camera_);
    view_->setViewport({0, 0, width, height});
    view_->setVisibleLayers(kFlutterLayerMask, kFlutterLayerMask);
    view_->setPostProcessingEnabled(false);
    view_->setShadowingEnabled(false);

    // Orthographic projection (identity — NDC quad)
    camera_->setProjection(filament::Camera::Projection::ORTHO,
                           -1, 1, -1, 1, -1, 1);

    // Allocate RGBA texture (initially blank)
    texture_ = filament::Texture::Builder()
        .width(width)
        .height(height)
        .levels(1)
        .format(filament::Texture::InternalFormat::RGBA8)
        .sampler(filament::Texture::Sampler::SAMPLER_2D)
        .build(*engine_);

    initialized_ = true;
    SE_LOG_INFO("FlutterOverlayRenderer initialized (Filament overlay, layer 5)");
}

void FlutterOverlayRenderer::Shutdown() {
    if (!initialized_) return;

    if (quad_entity_ && scene_) {
        scene_->remove(quad_entity_);
        engine_->destroy(quad_entity_);
        quad_entity_ = {};
    }
    if (vertex_buffer_) { engine_->destroy(vertex_buffer_); vertex_buffer_ = nullptr; }
    if (index_buffer_)  { engine_->destroy(index_buffer_);  index_buffer_  = nullptr; }
    if (material_instance_) { engine_->destroy(material_instance_); material_instance_ = nullptr; }
    if (material_)      { engine_->destroy(material_);      material_      = nullptr; }
    if (texture_)       { engine_->destroy(texture_);       texture_       = nullptr; }
    if (view_)          { engine_->destroy(view_);          view_          = nullptr; }
    if (scene_)         { engine_->destroy(scene_);         scene_         = nullptr; }
    if (camera_) {
        auto entity = camera_->getEntity();
        engine_->destroyCameraComponent(entity);
        utils::EntityManager::get().destroy(entity);
        camera_ = nullptr;
    }

    initialized_ = false;
    SE_LOG_INFO("FlutterOverlayRenderer shut down");
}

// ── Frame Operations ──────────────────────────────────────────

void FlutterOverlayRenderer::UpdateTexture(const uint8_t* pixelData,
                                           uint32_t width,
                                           uint32_t height) {
    if (!initialized_ || !pixelData || !engine_) return;

    // Recreate texture if size changed
    if (width != width_ || height != height_) {
        OnResize(width, height);
    }

    // Upload pixel data to Filament texture
    size_t dataSize = static_cast<size_t>(width_) * height_ * 4;
    auto* copy = new uint8_t[dataSize];
    std::memcpy(copy, pixelData, dataSize);

    filament::Texture::PixelBufferDescriptor buffer(
        copy, dataSize,
        filament::Texture::Format::RGBA,
        filament::Texture::Type::UBYTE,
        [](void* buf, size_t, void*) { delete[] static_cast<uint8_t*>(buf); },
        nullptr
    );

    texture_->setImage(*engine_, 0, std::move(buffer));
}

void FlutterOverlayRenderer::OnResize(uint32_t width, uint32_t height) {
    if (!initialized_) return;
    width_  = width;
    height_ = height;

    view_->setViewport({0, 0, width, height});

    // Recreate texture with new dimensions
    if (texture_) {
        engine_->destroy(texture_);
    }
    texture_ = filament::Texture::Builder()
        .width(width)
        .height(height)
        .levels(1)
        .format(filament::Texture::InternalFormat::RGBA8)
        .sampler(filament::Texture::Sampler::SAMPLER_2D)
        .build(*engine_);
}

filament::View* FlutterOverlayRenderer::GetView() const {
    return view_;
}

}  // namespace se::ui::flutter
