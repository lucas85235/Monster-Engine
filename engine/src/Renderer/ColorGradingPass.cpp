#include "engine/renderer/ColorGradingPass.h"
#include "engine/Shader.h"
#include "engine/Log.h"

#include <imgui.h>

namespace se {

void ColorGradingPass::Init(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
    
    SE_LOG_INFO("[ColorGradingPass] Initializing {}x{}", width, height);
    
    shader_ = Shader::CreateFromFiles(
        "assets/shaders/postprocess/fullscreen_quad.vert",
        "assets/shaders/postprocess/color_grading.frag"
    );
    
    if (!shader_ || shader_->getID() == 0) {
        SE_LOG_ERROR("[ColorGradingPass] Failed to load shader");
        return;
    }
    
    float quadVertices[] = {
        -1.0f,  1.0f, 0.0f, 1.0f,
        -1.0f, -1.0f, 0.0f, 0.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
        -1.0f,  1.0f, 0.0f, 1.0f,
         1.0f, -1.0f, 1.0f, 0.0f,
         1.0f,  1.0f, 1.0f, 1.0f
    };
    
    glGenVertexArrays(1, &quadVAO_);
    glGenBuffers(1, &quadVBO_);
    glBindVertexArray(quadVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);
    
    SE_LOG_INFO("[ColorGradingPass] Initialized successfully");
}

void ColorGradingPass::Shutdown() {
    if (quadVAO_ != 0) {
        glDeleteVertexArrays(1, &quadVAO_);
        quadVAO_ = 0;
    }
    if (quadVBO_ != 0) {
        glDeleteBuffers(1, &quadVBO_);
        quadVBO_ = 0;
    }
    shader_.reset();
}

void ColorGradingPass::Resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
}

void ColorGradingPass::Execute(GLuint inputTexture, GLuint outputFBO) {
    if (!shader_ || !enabled_) return;
    
    glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
    // Viewport is set by PostProcessPipeline - do NOT override here
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    
    shader_->bind();
    
    shader_->setInt("uSceneTexture", 0);
    shader_->setFloat("uExposure", exposure_);
    shader_->setFloat("uContrast", contrast_);
    shader_->setFloat("uSaturation", saturation_);
    shader_->setFloat("uBrightness", brightness_);
    shader_->setFloat("uTemperature", temperature_);
    shader_->setFloat("uTint", tint_);
    shader_->setFloat("uShadows", shadows_);
    shader_->setFloat("uHighlights", highlights_);
    shader_->setFloat("uVignetteIntensity", vignetteIntensity_);
    shader_->setFloat("uVignetteFalloff", vignetteFalloff_);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    
    RenderQuad();
    
    glEnable(GL_DEPTH_TEST);
}

void ColorGradingPass::RenderUI() {
    ImGui::SliderFloat("Exposure", &exposure_, 0.1f, 5.0f);
    ImGui::SliderFloat("Contrast", &contrast_, 0.5f, 2.0f);
    ImGui::SliderFloat("Saturation", &saturation_, 0.0f, 2.0f);
    ImGui::SliderFloat("Brightness", &brightness_, -0.5f, 0.5f);
    ImGui::Separator();
    ImGui::SliderFloat("Temperature", &temperature_, -1.0f, 1.0f);
    ImGui::SliderFloat("Tint", &tint_, -1.0f, 1.0f);
    ImGui::Separator();
    ImGui::SliderFloat("Shadows", &shadows_, -1.0f, 1.0f);
    ImGui::SliderFloat("Highlights", &highlights_, -1.0f, 1.0f);
    ImGui::Separator();
    ImGui::SliderFloat("Vignette", &vignetteIntensity_, 0.0f, 1.0f);
    ImGui::SliderFloat("Vignette Falloff", &vignetteFalloff_, 0.1f, 1.0f);
}

void ColorGradingPass::RenderQuad() {
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

} // namespace se
