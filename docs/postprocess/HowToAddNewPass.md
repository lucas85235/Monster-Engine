# How to Add a New Post-Process Pass

This guide walks through creating a custom post-processing pass and integrating it into the pipeline.

---

## Step 1: Create the Header

Create a new header file in `engine/include/engine/renderer/`:

```cpp
// MyCustomPass.h
#pragma once

#include "engine/renderer/PostProcessPass.h"
#include "engine/Shader.h"
#include <memory>

namespace se {

class MyCustomPass : public PostProcessPass {
public:
    MyCustomPass() = default;
    ~MyCustomPass() override = default;

    void Init(uint32_t width, uint32_t height) override;
    void Shutdown() override;
    void Resize(uint32_t width, uint32_t height) override;
    void Execute(GLuint inputTexture, GLuint outputFBO) override;
    void RenderUI() override;
    const char* GetName() const override { return "MyCustom"; }

    // Custom properties
    float GetIntensity() const { return intensity_; }
    void SetIntensity(float i) { intensity_ = i; }

private:
    void RenderQuad();
    void CreateResources();
    void DestroyResources();

    std::shared_ptr<Shader> shader_;
    
    // Custom parameters
    float intensity_ = 1.0f;

    // Fullscreen quad
    GLuint quadVAO_ = 0;
    GLuint quadVBO_ = 0;
};

}  // namespace se
```

---

## Step 2: Create the Implementation

```cpp
// MyCustomPass.cpp
#include "engine/renderer/MyCustomPass.h"
#include "engine/Log.h"
#include <imgui.h>

namespace se {

void MyCustomPass::Init(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;

    // Load shader
    shader_ = std::make_shared<Shader>();
    shader_->LoadFromFiles(
        "assets/shaders/fullscreen_quad.vert",
        "assets/shaders/my_custom.frag"
    );

    // Create fullscreen quad
    float quadVertices[] = {
        // positions   // texcoords
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
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glBindVertexArray(0);

    Logger::Info("[MyCustomPass] Initialized at {}x{}", width, height);
}

void MyCustomPass::Shutdown() {
    if (quadVAO_) {
        glDeleteVertexArrays(1, &quadVAO_);
        glDeleteBuffers(1, &quadVBO_);
        quadVAO_ = 0;
        quadVBO_ = 0;
    }
    shader_.reset();
}

void MyCustomPass::Resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
    // Recreate any resolution-dependent resources here
}

void MyCustomPass::Execute(GLuint inputTexture, GLuint outputFBO) {
    if (!enabled_ || !shader_) return;

    // Bind output
    glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
    glViewport(0, 0, width_, height_);
    glClear(GL_COLOR_BUFFER_BIT);

    // Use shader
    shader_->Bind();
    shader_->SetInt("uInputTexture", 0);
    shader_->SetFloat("uIntensity", intensity_);

    // Bind input texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, inputTexture);

    // Draw fullscreen quad
    RenderQuad();

    shader_->Unbind();
}

void MyCustomPass::RenderQuad() {
    glBindVertexArray(quadVAO_);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void MyCustomPass::RenderUI() {
    if (ImGui::CollapsingHeader("My Custom Pass")) {
        ImGui::Checkbox("Enabled##mycustom", &enabled_);
        ImGui::SliderFloat("Intensity##mycustom", &intensity_, 0.0f, 2.0f);
    }
}

}  // namespace se
```

---

## Step 3: Create the Shader

Create the fragment shader:

```glsl
// assets/shaders/my_custom.frag
#version 450 core

in vec2 TexCoords;
out vec4 FragColor;

uniform sampler2D uInputTexture;
uniform float uIntensity;

void main() {
    vec3 color = texture(uInputTexture, TexCoords).rgb;
    
    // Your custom effect here
    // Example: simple color adjustment
    color *= uIntensity;
    
    FragColor = vec4(color, 1.0);
}
```

Reuse the standard vertex shader:

```glsl
// assets/shaders/fullscreen_quad.vert
#version 450 core

layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoords;

out vec2 TexCoords;

void main() {
    TexCoords = aTexCoords;
    gl_Position = vec4(aPos, 0.0, 1.0);
}
```

---

## Step 4: Add to CMake

Add the new files to your CMakeLists.txt:

```cmake
set(RENDERER_SOURCES
    # ... existing sources
    src/Renderer/MyCustomPass.cpp
)

set(RENDERER_HEADERS
    # ... existing headers
    include/engine/renderer/MyCustomPass.h
)
```

---

## Step 5: Use the Pass

```cpp
#include "engine/renderer/MyCustomPass.h"

// In your layer's OnAttach:
void OnAttach() override {
    auto* pipeline = GetPostProcessPipeline();
    
    // Add after bloom, before tonemapping
    auto* myPass = pipeline->AddPass<se::MyCustomPass>();
    myPass->SetIntensity(1.5f);
}
```

---

## Pass Execution Flow

When `Execute()` is called:

1. `outputFBO` is one of the ping-pong framebuffers
2. `inputTexture` is the result of the previous pass
3. Your pass renders to `outputFBO`
4. The next pass receives your output as input

```mermaid
graph LR
    P1[Previous Pass] -->|inputTexture| YOUR[Your Pass]
    YOUR -->|outputFBO| P2[Next Pass]
```

---

## Tips

1. **Check enabled_**: Always check `if (!enabled_) return;`
2. **Preserve alpha**: If needed, preserve the alpha channel
3. **Resolution**: Use `width_` and `height_` from the base class
4. **Viewport**: Always set viewport in Execute
5. **Cleanup**: Implement Shutdown properly

---

## Advanced: Resolution Scaling

For expensive effects, work at lower resolution:

```cpp
void Init(uint32_t width, uint32_t height) override {
    width_ = width;
    height_ = height;
    
    // Work at half resolution
    workWidth_ = width / 2;
    workHeight_ = height / 2;
    
    // Create resolution-scaled framebuffer
    glGenFramebuffers(1, &workFBO_);
    // ...
}

void Execute(GLuint inputTexture, GLuint outputFBO) override {
    // 1. Downsample to work resolution
    // 2. Process at work resolution  
    // 3. Upsample to output
}
```

---

## Advanced: Multi-Pass Effects

For complex effects requiring multiple passes:

```cpp
void Execute(GLuint inputTexture, GLuint outputFBO) override {
    // Pass 1: Horizontal blur
    glBindFramebuffer(GL_FRAMEBUFFER, tempFBO_);
    horizontalBlurShader_->Bind();
    // ... render
    
    // Pass 2: Vertical blur to output
    glBindFramebuffer(GL_FRAMEBUFFER, outputFBO);
    verticalBlurShader_->Bind();
    glBindTexture(GL_TEXTURE_2D, tempTexture_);
    // ... render
}
```

---

## See Also

- [Post-Processing Overview](Overview.md)
- [Bloom](Bloom.md) - Example multi-pass effect
- [Shader System](../resources/Shaders.md)
