#include "engine/renderer/TonemappingPass.h"
#include "engine/Log.h"

#include <imgui.h>

namespace se {

void TonemappingPass::Init(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;

    SE_LOG_INFO("[TonemappingPass] Initializing");

    tonemapShader_ = Shader::CreateFromFiles(
        "assets/shaders/postprocess/fullscreen_quad.vert",
        "assets/shaders/postprocess/tonemap.frag"
    );

    float quadVertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f
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
}

void TonemappingPass::Shutdown() {
    if (quadVAO_) {
        glDeleteVertexArrays(1, &quadVAO_);
        quadVAO_ = 0;
    }
    if (quadVBO_) {
        glDeleteBuffers(1, &quadVBO_);
        quadVBO_ = 0;
    }
    tonemapShader_.reset();
}

void TonemappingPass::Resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
}

void TonemappingPass::Execute(GLuint inputTexture, GLuint outputFBO) {
    glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
    // NOTE: Viewport is NOT set here - caller is responsible for setting correct viewport
    // This allows external framebuffers with different sizes to work correctly
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    tonemapShader_->bind();
    tonemapShader_->setInt("uHDRTexture", 0);
    tonemapShader_->setFloat("uExposure", exposure_);
    tonemapShader_->setFloat("uGamma", gamma_);
    tonemapShader_->setInt("uTonemapOperator", static_cast<int>(tonemapOp_));

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);

    RenderQuad();

    glEnable(GL_DEPTH_TEST);
}

void TonemappingPass::RenderUI() {
    ImGui::SliderFloat("Exposure##tonemap", &exposure_, 0.1f, 5.0f);
    ImGui::SliderFloat("Gamma", &gamma_, 1.0f, 3.0f);
    
    const char* operators[] = { "ACES", "Reinhard", "Uncharted2", "Neutral", "AgX" };
    int currentOp = static_cast<int>(tonemapOp_);
    if (ImGui::Combo("Operator", &currentOp, operators, IM_ARRAYSIZE(operators))) {
        tonemapOp_ = static_cast<TonemapOperator>(currentOp);
    }
}

void TonemappingPass::RenderQuad() {
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

}  // namespace se
