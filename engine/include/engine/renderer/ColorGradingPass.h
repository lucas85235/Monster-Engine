#pragma once
/**
 * ColorGradingPass.h - Post-process color grading
 * 
 * Adjusts exposure, contrast, saturation, white balance, and vignette.
 */

#include "engine/renderer/PostProcessPass.h"
#include <memory>

namespace se {

class Shader;

class ColorGradingPass : public PostProcessPass {
public:
    ColorGradingPass() = default;
    ~ColorGradingPass() override = default;
    
    void Init(uint32_t width, uint32_t height) override;
    void Shutdown() override;
    void Resize(uint32_t width, uint32_t height) override;
    void Execute(GLuint inputTexture, GLuint outputFBO) override;
    void RenderUI() override;
    const char* GetName() const override { return "Color Grading"; }
    
    // Public settings access
    float& Exposure() { return exposure_; }
    float& Contrast() { return contrast_; }
    float& Saturation() { return saturation_; }
    float& Brightness() { return brightness_; }
    float& Temperature() { return temperature_; }
    float& Tint() { return tint_; }
    float& Shadows() { return shadows_; }
    float& Highlights() { return highlights_; }
    float& VignetteIntensity() { return vignetteIntensity_; }
    float& VignetteFalloff() { return vignetteFalloff_; }

private:
    void RenderQuad();
    
    std::shared_ptr<Shader> shader_;
    GLuint quadVAO_ = 0;
    GLuint quadVBO_ = 0;
    
    // Settings
    float exposure_ = 1.0f;
    float contrast_ = 1.0f;
    float saturation_ = 1.0f;
    float brightness_ = 0.0f;
    float temperature_ = 0.0f;
    float tint_ = 0.0f;
    float shadows_ = 0.0f;
    float highlights_ = 0.0f;
    float vignetteIntensity_ = 0.0f;
    float vignetteFalloff_ = 0.5f;
};

} // namespace se
