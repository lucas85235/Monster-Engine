#pragma once

#include <filament/Options.h>
#include <filament/Renderer.h>

namespace se {

class FilamentRenderer;

struct RenderSettingsState {
    float clearColor[4] = {0.1f, 0.1f, 0.15f, 1.0f};
    bool  clearEnabled  = true;
    bool  clearDiscard  = true;

    bool shadowingEnabled             = true;
    bool screenSpaceRefractionEnabled = true;
    bool postProcessingEnabled        = true;
    bool frontFaceWindingInverted     = false;
    bool frustumCullingEnabled        = true;

    int antiAliasing = static_cast<int>(filament::AntiAliasing::FXAA);
    int dithering    = static_cast<int>(filament::Dithering::TEMPORAL);
    int shadowType   = static_cast<int>(filament::ShadowType::PCF);
    int hdrQuality   = static_cast<int>(filament::QualityLevel::HIGH);

    filament::DynamicResolutionOptions       dynamicResOptions;
    filament::MultiSampleAntiAliasingOptions msaaOptions;
    filament::TemporalAntiAliasingOptions    taaOptions;
    filament::AmbientOcclusionOptions        aoOptions;
    filament::ScreenSpaceReflectionsOptions  ssrOptions;
    filament::BloomOptions                   bloomOptions;
    filament::FogOptions                     fogOptions;
    filament::VignetteOptions                vignetteOptions;
    filament::GuardBandOptions               guardBandOptions;
    filament::VsmShadowOptions               vsmShadowOptions;
    filament::SoftShadowOptions              softShadowOptions;

    filament::Renderer::FrameRateOptions frameRateOptions;
};

/**
 * Central runtime state for Filament renderer/view options.
 *
 * Gameplay layers can edit this state, while this system is the single
 * authority that reads/writes Filament APIs.
 */
class RenderSettingsSystem {
public:
    RenderSettingsSystem() = default;

    void Init(FilamentRenderer* renderer);
    void Shutdown();

    bool IsInitialized() const {
        return initialized_;
    }

    RenderSettingsState& GetMutableSettings() {
        return settings_;
    }

    const RenderSettingsState& GetSettings() const {
        return settings_;
    }

    void SyncFromRenderer();

    void MarkDirty() {
        dirty_ = true;
    }

    bool IsDirty() const {
        return dirty_;
    }

    void Apply();

private:
    FilamentRenderer*   renderer_    = nullptr;
    RenderSettingsState settings_{};
    bool                initialized_ = false;
    bool                dirty_       = false;
};

} // namespace se
