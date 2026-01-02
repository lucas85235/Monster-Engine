# Adding a New Post-Process Pass Tutorial

This tutorial walks through creating a simple vignette post-processing effect.

---

## Goal

Create a `VignettePass` that darkens the edges of the screen.

---

## Step 1: Create the Header

Create `engine/include/engine/renderer/VignettePass.h`:

```cpp
#pragma once

#include "engine/renderer/PostProcessPass.h"
#include "engine/Shader.h"
#include <memory>

namespace se {

class VignettePass : public PostProcessPass {
public:
    VignettePass() = default;
    ~VignettePass() override = default;

    void Init(uint32_t width, uint32_t height) override;
    void Shutdown() override;
    void Resize(uint32_t width, uint32_t height) override;
    void Execute(GLuint inputTexture, GLuint outputFBO) override;
    void RenderUI() override;
    const char* GetName() const override { return "Vignette"; }

    float GetIntensity() const { return intensity_; }
    void SetIntensity(float i) { intensity_ = i; }
    
    float GetFalloff() const { return falloff_; }
    void SetFalloff(float f) { falloff_ = f; }

private:
    void RenderQuad();

    std::shared_ptr<Shader> shader_;
    
    float intensity_ = 0.5f;
    float falloff_ = 0.3f;

    GLuint quadVAO_ = 0;
    GLuint quadVBO_ = 0;
};

}  // namespace se
```

---

## Step 2: Create the Implementation

Create `engine/src/Renderer/VignettePass.cpp`:

```cpp
#include "engine/renderer/VignettePass.h"
#include "engine/Log.h"
#include <imgui.h>
#include <glad/glad.h>

namespace se {

void VignettePass::Init(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;

    // Load shader
    shader_ = std::make_shared<Shader>();
    shader_->LoadFromFiles(
        "assets/shaders/postprocess/fullscreen.vert",
        "assets/shaders/postprocess/vignette.frag"
    );

    // Create fullscreen quad
    float quadVertices[] = {
        -1.0f,  1.0f,  0.0f, 1.0f,
        -1.0f, -1.0f,  0.0f, 0.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
        -1.0f,  1.0f,  0.0f, 1.0f,
         1.0f, -1.0f,  1.0f, 0.0f,
         1.0f,  1.0f,  1.0f, 1.0f,
    };

    glGenVertexArrays(1, &quadVAO_);
    glGenBuffers(1, &quadVBO_);
    glBindVertexArray(quadVAO_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVBO_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), 
                 quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 
                          4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 
                          4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    Logger::Info("[VignettePass] Initialized");
}

void VignettePass::Shutdown() {
    if (quadVAO_) {
        glDeleteVertexArrays(1, &quadVAO_);
        glDeleteBuffers(1, &quadVBO_);
        quadVAO_ = 0;
        quadVBO_ = 0;
    }
    shader_.reset();
}

void VignettePass::Resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
}

void VignettePass::Execute(GLuint inputTexture, GLuint outputFBO) {
    if (!enabled_ || !shader_) return;

    glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
    glViewport(0, 0, width_, height_);

    shader_->Bind();
    shader_->SetInt("uInputTexture", 0);
    shader_->SetFloat("uIntensity", intensity_);
    shader_->SetFloat("uFalloff", falloff_);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);

    RenderQuad();

    shader_->Unbind();
}

void VignettePass::RenderQuad() {
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void VignettePass::RenderUI() {
    if (ImGui::CollapsingHeader("Vignette")) {
        ImGui::Checkbox("Enabled##vignette", &enabled_);
        ImGui::SliderFloat("Intensity##vignette", &intensity_, 0.0f, 1.0f);
        ImGui::SliderFloat("Falloff##vignette", &falloff_, 0.0f, 1.0f);
    }
}

}  // namespace se
```

---

## Step 3: Create the Shader

Create `assets/shaders/postprocess/vignette.frag`:

```glsl
#version 450 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D uInputTexture;
uniform float uIntensity;
uniform float uFalloff;

void main() {
    vec3 color = texture(uInputTexture, TexCoords).rgb;
    
    // Calculate distance from center
    vec2 center = vec2(0.5);
    float dist = distance(TexCoords, center);
    
    // Apply vignette
    float vignette = smoothstep(uFalloff, 1.0 - uFalloff, dist);
    color *= 1.0 - (vignette * uIntensity);
    
    FragColor = vec4(color, 1.0);
}
```

---

## Step 4: Add to CMake

In your CMakeLists.txt:

```cmake
set(RENDERER_SOURCES
    ${RENDERER_SOURCES}
    src/Renderer/VignettePass.cpp
)
```

---

## Step 5: Use the Pass

```cpp
#include "engine/renderer/VignettePass.h"

void OnAttach() override {
    auto* pipeline = GetPostProcessPipeline();
    
    // Add after bloom, before tonemapping
    auto* vignette = pipeline->AddPass<se::VignettePass>();
    vignette->SetIntensity(0.4f);
    vignette->SetFalloff(0.3f);
}
```

---

## Result

The vignette effect will darken the edges of your screen, creating a cinematic look.

---

## See Also

- [Post-Processing Overview](../postprocess/Overview.md)
- [How to Add New Pass](../postprocess/HowToAddNewPass.md)
