#include "engine/renderer/RenderSettingsSystem.h"

#include "engine/renderer/FilamentRenderer.h"

#include <filament/Renderer.h>
#include <filament/View.h>

#include <spdlog/spdlog.h>

#include <algorithm>

namespace se {

void RenderSettingsSystem::Init(FilamentRenderer* renderer) {
    if (initialized_) {
        spdlog::warn("RenderSettingsSystem::Init called but already initialized. Ignoring.");
        return;
    }

    renderer_ = renderer;
    if (!renderer_) {
        spdlog::error("RenderSettingsSystem::Init called with null renderer.");
        return;
    }

    SyncFromRenderer();
    initialized_ = true;
    dirty_       = false;

    spdlog::info("RenderSettingsSystem initialized.");
}

void RenderSettingsSystem::Shutdown() {
    if (!initialized_) {
        return;
    }

    renderer_    = nullptr;
    initialized_ = false;
    dirty_       = false;

    spdlog::info("RenderSettingsSystem shut down.");
}

void RenderSettingsSystem::SyncFromRenderer() {
    if (!renderer_) {
        return;
    }

    auto* view      = renderer_->GetView();
    auto* renderer  = renderer_->GetRenderer();
    if (!view || !renderer) {
        return;
    }

    const auto& clear = renderer->getClearOptions();
    settings_.clearColor[0] = clear.clearColor[0];
    settings_.clearColor[1] = clear.clearColor[1];
    settings_.clearColor[2] = clear.clearColor[2];
    settings_.clearColor[3] = clear.clearColor[3];
    settings_.clearEnabled  = clear.clear;
    settings_.clearDiscard  = clear.discard;

    settings_.postProcessingEnabled        = view->isPostProcessingEnabled();
    settings_.shadowingEnabled             = view->isShadowingEnabled();
    settings_.screenSpaceRefractionEnabled = view->isScreenSpaceRefractionEnabled();
    settings_.frontFaceWindingInverted     = view->isFrontFaceWindingInverted();
    settings_.frustumCullingEnabled        = view->isFrustumCullingEnabled();
    settings_.antiAliasing                 = static_cast<int>(view->getAntiAliasing());
    settings_.dithering                    = static_cast<int>(view->getDithering());
    settings_.shadowType                   = static_cast<int>(view->getShadowType());
    settings_.hdrQuality                   = static_cast<int>(view->getRenderQuality().hdrColorBuffer);

    settings_.dynamicResOptions = view->getDynamicResolutionOptions();
    settings_.msaaOptions       = view->getMultiSampleAntiAliasingOptions();
    settings_.taaOptions        = view->getTemporalAntiAliasingOptions();
    settings_.aoOptions         = view->getAmbientOcclusionOptions();
    settings_.ssrOptions        = view->getScreenSpaceReflectionsOptions();
    settings_.bloomOptions      = view->getBloomOptions();
    settings_.fogOptions        = view->getFogOptions();
    settings_.vignetteOptions   = view->getVignetteOptions();
    settings_.guardBandOptions  = view->getGuardBandOptions();
    settings_.vsmShadowOptions  = view->getVsmShadowOptions();
    settings_.softShadowOptions = view->getSoftShadowOptions();

    // Filament does not expose a getter for frame-rate options.
    settings_.frameRateOptions = filament::Renderer::FrameRateOptions{};

    dirty_ = false;
}

void RenderSettingsSystem::Apply() {
    if (!renderer_) {
        return;
    }

    auto* view     = renderer_->GetView();
    auto* renderer = renderer_->GetRenderer();
    if (!view || !renderer) {
        return;
    }

    auto& s = settings_;

    s.antiAliasing = std::clamp(s.antiAliasing, 0, 1);
    s.dithering    = std::clamp(s.dithering, 0, 1);
    s.shadowType   = std::clamp(s.shadowType, 0, 3);
    s.hdrQuality   = std::clamp(s.hdrQuality, 0, 3);

    filament::Renderer::ClearOptions clear = renderer->getClearOptions();
    clear.clearColor = {s.clearColor[0], s.clearColor[1], s.clearColor[2], s.clearColor[3]};
    clear.clear      = s.clearEnabled;
    clear.discard    = s.clearDiscard;
    renderer->setClearOptions(clear);
    renderer->setFrameRateOptions(s.frameRateOptions);

    view->setPostProcessingEnabled(s.postProcessingEnabled);
    view->setShadowingEnabled(s.shadowingEnabled);
    view->setScreenSpaceRefractionEnabled(s.screenSpaceRefractionEnabled);
    view->setFrontFaceWindingInverted(s.frontFaceWindingInverted);
    view->setFrustumCullingEnabled(s.frustumCullingEnabled);
    view->setAntiAliasing(static_cast<filament::AntiAliasing>(s.antiAliasing));
    view->setDithering(static_cast<filament::Dithering>(s.dithering));
    view->setShadowType(static_cast<filament::ShadowType>(s.shadowType));
    view->setDynamicResolutionOptions(s.dynamicResOptions);
    view->setMultiSampleAntiAliasingOptions(s.msaaOptions);
    view->setTemporalAntiAliasingOptions(s.taaOptions);
    view->setAmbientOcclusionOptions(s.aoOptions);
    view->setScreenSpaceReflectionsOptions(s.ssrOptions);
    view->setBloomOptions(s.bloomOptions);
    view->setFogOptions(s.fogOptions);
    view->setVignetteOptions(s.vignetteOptions);
    view->setGuardBandOptions(s.guardBandOptions);
    view->setVsmShadowOptions(s.vsmShadowOptions);
    view->setSoftShadowOptions(s.softShadowOptions);

    auto renderQuality         = view->getRenderQuality();
    renderQuality.hdrColorBuffer = static_cast<filament::QualityLevel>(s.hdrQuality);
    view->setRenderQuality(renderQuality);

    dirty_ = false;
}

} // namespace se
