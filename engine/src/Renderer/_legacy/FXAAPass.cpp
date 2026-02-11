#include "engine/renderer/FXAAPass.h"
#include "engine/Shader.h"
#include "engine/Log.h"

#include <imgui.h>

namespace se {

void FXAAPass::Init(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
    
    SE_LOG_INFO("[FXAAPass] Initializing {}x{}", width, height);
    
    shader_ = Shader::CreateFromFiles(
        "assets/shaders/postprocess/fullscreen_quad.vert",
        "assets/shaders/postprocess/fxaa.frag"
    );
    
    if (!shader_ || shader_->getID() == 0) {
        SE_LOG_ERROR("[FXAAPass] Failed to load shader");
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
    
    SE_LOG_INFO("[FXAAPass] Initialized successfully");
}

void FXAAPass::Shutdown() {
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

void FXAAPass::Resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
}

void FXAAPass::Execute(GLuint inputTexture, GLuint outputFBO) {
    if (!shader_ || !enabled_) return;
    
    glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
    // Viewport is set by PostProcessPipeline - do NOT override here
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    
    shader_->bind();
    shader_->setInt("uSceneTexture", 0);
    shader_->setVec2("uTexelSize", glm::vec2(1.0f / width_, 1.0f / height_));
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);
    
    RenderQuad();
    
    glEnable(GL_DEPTH_TEST);
}

void FXAAPass::RenderUI() {
    // No additional settings for FXAA
}

void FXAAPass::RenderQuad() {
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

} // namespace se
