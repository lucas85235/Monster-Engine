#include "engine/renderer/FilamentRenderer.h"
#include "engine/renderer/FilamentContext.h"

#include <filament/Engine.h>
#include <filament/Renderer.h>
#include <filament/Scene.h>
#include <filament/View.h>
#include <filament/Camera.h>
#include <filament/SwapChain.h>
#include <filament/Viewport.h>
#include <filament/Options.h>

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cmath>

namespace se {

FilamentRenderer::FilamentRenderer(FilamentContext& context)
    : context_(context) {
    spdlog::info("FilamentRenderer created.");
}

FilamentRenderer::~FilamentRenderer() {
    spdlog::info("FilamentRenderer destroyed.");
}

bool FilamentRenderer::BeginFrame() {
    auto* renderer = context_.GetRenderer();
    auto* swapChain = context_.GetSwapChain();

    if (!renderer || !swapChain) {
        return false;
    }

    return renderer->beginFrame(swapChain);
}

void FilamentRenderer::EndFrame() {
    auto* renderer = context_.GetRenderer();
    auto* view = context_.GetView();

    if (!renderer || !view) {
        return;
    }

    // Render the main view
    renderer->render(view);

    // Render overlay views (UI/debug) on top of the main view.
    for (filament::View* overlayView : overlay_views_) {
        if (!overlayView) continue;
        renderer->render(overlayView);
    }

    // End the frame — this presents to the swap chain
    renderer->endFrame();
}

void FilamentRenderer::SetClearColor(float r, float g, float b, float a) {
    auto* renderer = context_.GetRenderer();
    if (!renderer) return;

    renderer->setClearOptions({
        .clearColor = { r, g, b, a },
        .clear = true,
    });
}

void FilamentRenderer::SetCameraProjection(double fovDegrees, double aspect,
                                            double near, double far) {
    auto* camera = context_.GetCamera();
    if (!camera) return;

    camera->setProjection(fovDegrees, aspect, near, far,
                          filament::Camera::Fov::VERTICAL);
}

void FilamentRenderer::SetCameraLookAt(const luma::Vector3& eye,
                                        const luma::Vector3& center,
                                        const luma::Vector3& up) {
    auto* camera = context_.GetCamera();
    if (!camera) return;

    camera->lookAt(
        {eye.x, eye.y, eye.z},
        {center.x, center.y, center.z},
        {up.x, up.y, up.z}
    );
}

void FilamentRenderer::SetViewport(uint32_t x, uint32_t y,
                                    uint32_t width, uint32_t height) {
    auto* view = context_.GetView();
    if (!view) return;

    view->setViewport({
        static_cast<int32_t>(x),
        static_cast<int32_t>(y),
        width,
        height
    });
}

filament::Engine* FilamentRenderer::GetEngine() const {
    return context_.GetEngine();
}

filament::Scene* FilamentRenderer::GetScene() const {
    return context_.GetScene();
}

filament::View* FilamentRenderer::GetView() const {
    return context_.GetView();
}

filament::Camera* FilamentRenderer::GetCamera() const {
    return context_.GetCamera();
}

filament::Renderer* FilamentRenderer::GetRenderer() const {
    return context_.GetRenderer();
}

void FilamentRenderer::RegisterOverlayView(filament::View* view) {
    if (!view) return;
    if (std::find(overlay_views_.begin(), overlay_views_.end(), view) != overlay_views_.end()) {
        return;
    }
    overlay_views_.push_back(view);
}

void FilamentRenderer::UnregisterOverlayView(filament::View* view) {
    if (!view) return;
    overlay_views_.erase(std::remove(overlay_views_.begin(), overlay_views_.end(), view),
                         overlay_views_.end());
}

} // namespace se
