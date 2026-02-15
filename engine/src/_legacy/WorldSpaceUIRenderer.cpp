#include "engine/ui/native/world/WorldSpaceUIRenderer.h"
#include "engine/ui/native/world/WorldSpaceUIComponent.h"
#include "engine/ui/native/world/WorldSpaceUIElement.h"
#include "engine/ui/native/font/UIFont.h"
#include "engine/ui/native/font/UIFontManager.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/Camera.h"
#include "engine/Shader.h"
#include "engine/Log.h"
#include "Engine.h"

#include <glad/glad.h>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace se {

WorldSpaceUIRenderer& WorldSpaceUIRenderer::Get() {
    static WorldSpaceUIRenderer instance;
    return instance;
}

void WorldSpaceUIRenderer::Initialize() {
    if (initialized_) return;
    InitializeResources();
    initialized_ = true;
    SE_LOG_INFO("[WorldSpaceUIRenderer] Initialized");
}

void WorldSpaceUIRenderer::Shutdown() {
    if (!initialized_) return;
    
    if (textVao_) glDeleteVertexArrays(1, &textVao_);
    if (textVbo_) glDeleteBuffers(1, &textVbo_);
    if (quadVao_) glDeleteVertexArrays(1, &quadVao_);
    if (quadVbo_) glDeleteBuffers(1, &quadVbo_);
    
    textVao_ = textVbo_ = quadVao_ = quadVbo_ = 0;
    textShader_.reset();
    uiShader_.reset();
    font_.reset();
    
    initialized_ = false;
    SE_LOG_INFO("[WorldSpaceUIRenderer] Shutdown");
}

void WorldSpaceUIRenderer::InitializeResources() {
    std::string vertPath = "assets/shaders/debug/debug_text.vert";
    std::string fragPath = "assets/shaders/debug/debug_text.frag";
    
    std::ifstream vertFile(vertPath);
    std::ifstream fragFile(fragPath);
    
    if (!vertFile.is_open() || !fragFile.is_open()) {
        SE_LOG_ERROR("[WorldSpaceUIRenderer] Failed to load text shader files");
        return;
    }
    
    std::stringstream vertStream, fragStream;
    vertStream << vertFile.rdbuf();
    fragStream << fragFile.rdbuf();
    
    textShader_ = std::make_shared<Shader>(vertStream.str(), fragStream.str());
    
    font_ = ui::UIFontManager::Get().LoadFont("assets/fonts/Roboto-Regular.ttf", 16);
    
    glGenVertexArrays(1, &textVao_);
    glGenBuffers(1, &textVbo_);
    
    glBindVertexArray(textVao_);
    glBindBuffer(GL_ARRAY_BUFFER, textVbo_);
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
    glBindVertexArray(0);
    
    const std::string quadVertSrc = R"(#version 330 core
        layout (location = 0) in vec2 aPosition;
        
        uniform mat4 uViewProjection;
        uniform vec3 uWorldPos;
        uniform vec3 uCameraRight;
        uniform vec3 uCameraUp;
        uniform float uScale;
        
        void main() {
            vec3 worldPos = uWorldPos 
                          + uCameraRight * aPosition.x * uScale
                          + uCameraUp * aPosition.y * uScale;
            gl_Position = uViewProjection * vec4(worldPos, 1.0);
        }
    )";
    
    const std::string quadFragSrc = R"(#version 330 core
        uniform vec4 uColor;
        out vec4 FragColor;
        
        void main() {
            FragColor = uColor;
        }
    )";
    
    uiShader_ = std::make_shared<Shader>(quadVertSrc, quadFragSrc);
    
    glGenVertexArrays(1, &quadVao_);
    glGenBuffers(1, &quadVbo_);
    
    glBindVertexArray(quadVao_);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo_);
    glBufferData(GL_ARRAY_BUFFER, 6 * 2 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    
    glBindVertexArray(0);
}

void WorldSpaceUIRenderer::Render(const ::Camera& camera, entt::registry& registry) {
    if (!initialized_) {
        Initialize();
    }
    
    if (!textShader_ || !font_ || !font_->IsLoaded()) return;
    
    auto view = registry.view<TransformComponent, WorldSpaceUIComponent>();
    if (view.size_hint() == 0) return;
    
    Vector3 camPos = camera.GetPosition();
    Matrix4 viewMat = camera.getViewMatrix();
    Matrix4 projMat = camera.getProjectionMatrix(16.0f / 9.0f);
    Matrix4 viewProjection = projMat * viewMat;
    
    Vector3 camRight = Vector3(viewMat[0][0], viewMat[1][0], viewMat[2][0]);
    Vector3 camUp = Vector3(viewMat[0][1], viewMat[1][1], viewMat[2][1]);
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    for (auto entity : view) {
        auto& transform = view.get<TransformComponent>(entity);
        auto& worldUI = view.get<WorldSpaceUIComponent>(entity);
        
        if (worldUI.elements.empty()) continue;
        
        Vector3 worldPos = transform.Position + worldUI.offset;
        float distance = glm::distance(camPos, worldPos);
        
        if (distance > worldUI.maxDistance) continue;
        
        float scale = worldUI.baseScale;
        if (worldUI.scaleByDistance && distance > 0.0f) {
            // Use smooth scaling: scale is 1.0 at referenceDistance
            // Closer = slightly larger, farther = slightly smaller
            // Using sqrt for gentler curve than linear inverse
            float ratio = worldUI.referenceDistance / glm::max(distance, 0.1f);
            // Apply square root for smoother scaling (less aggressive)
            float smoothFactor = glm::sqrt(ratio);
            // Blend towards 1.0 to reduce extreme values
            float blendedFactor = glm::mix(1.0f, smoothFactor, 0.5f);
            scale = glm::clamp(worldUI.baseScale * blendedFactor, 
                               worldUI.minScale, worldUI.maxScale);
        }
        
        WorldSpaceRenderContext ctx;
        ctx.camera = &camera;
        ctx.worldPosition = worldPos;
        ctx.cameraRight = camRight;
        ctx.cameraUp = camUp;
        ctx.scale = scale;
        ctx.distanceToCamera = distance;
        
        bool prevDepthTest = glIsEnabled(GL_DEPTH_TEST);
        if (!worldUI.depthTest) {
            glDisable(GL_DEPTH_TEST);
        }
        
        for (auto& element : worldUI.elements) {
            if (element && element->visible) {
                element->Render(ctx);
            }
        }
        
        if (!worldUI.depthTest && prevDepthTest) {
            glEnable(GL_DEPTH_TEST);
        }
    }
    
    glDisable(GL_BLEND);
}

void WorldSpaceText::Render(const WorldSpaceRenderContext& ctx) {
    auto& renderer = WorldSpaceUIRenderer::Get();
    auto shader = renderer.GetTextShader();
    auto font = renderer.GetFont();
    
    if (!shader || !font || text.empty()) return;
    
    Vector3 elementPos = ctx.worldPosition;
    elementPos += ctx.cameraRight * localOffset.x * ctx.scale * 0.01f;
    elementPos += ctx.cameraUp * localOffset.y * ctx.scale * 0.01f;
    
    Matrix4 viewProj = ctx.camera->getProjectionMatrix(16.0f / 9.0f) * ctx.camera->getViewMatrix();
    
    shader->bind();
    shader->setMat4("uViewProjection", viewProj);
    shader->setVec3("uCameraRight", ctx.cameraRight);
    shader->setVec3("uCameraUp", ctx.cameraUp);
    shader->setInt("uFontAtlas", 0);
    shader->setVec3("uColor", Vector3(color));
    
    float textScale = ctx.scale * fontSize * 0.001f;
    shader->setFloat("uScale", textScale);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font->GetAtlasTextureId());
    
    float totalWidth = 0.0f;
    if (centered) {
        for (char c : text) {
            const auto& glyph = font->GetGlyph(static_cast<uint32_t>(c));
            totalWidth += glyph.advance;
        }
    }
    
    float cursorX = centered ? -totalWidth * 0.5f : 0.0f;
    
    uint32_t textVao = 0, textVbo = 0;
    glGenVertexArrays(1, &textVao);
    glGenBuffers(1, &textVbo);
    
    glBindVertexArray(textVao);
    glBindBuffer(GL_ARRAY_BUFFER, textVbo);
    glBufferData(GL_ARRAY_BUFFER, 6 * 4 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    
    for (char c : text) {
        const auto& glyph = font->GetGlyph(static_cast<uint32_t>(c));
        
        float xpos = cursorX + glyph.bearingX;
        float ypos = -(glyph.height - glyph.bearingY);
        float w = glyph.width;
        float h = glyph.height;
        
        shader->setVec3("uWorldPos", elementPos);
        
        float vertices[6][4] = {
            {xpos,     ypos + h,   glyph.u0, glyph.v0},
            {xpos,     ypos,       glyph.u0, glyph.v1},
            {xpos + w, ypos,       glyph.u1, glyph.v1},
            
            {xpos,     ypos + h,   glyph.u0, glyph.v0},
            {xpos + w, ypos,       glyph.u1, glyph.v1},
            {xpos + w, ypos + h,   glyph.u1, glyph.v0}
        };
        
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
        glDrawArrays(GL_TRIANGLES, 0, 6);
        
        cursorX += glyph.advance;
    }
    
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &textVao);
    glDeleteBuffers(1, &textVbo);
    
    shader->unbind();
}

void WorldSpaceProgressBar::Render(const WorldSpaceRenderContext& ctx) {
    auto& renderer = WorldSpaceUIRenderer::Get();
    auto shader = renderer.GetUIShader();
    
    if (!shader) return;
    
    Vector3 elementPos = ctx.worldPosition;
    elementPos += ctx.cameraRight * localOffset.x * ctx.scale * 0.01f;
    elementPos += ctx.cameraUp * localOffset.y * ctx.scale * 0.01f;
    
    float scaledWidth = size.x * ctx.scale * 0.01f;
    float scaledHeight = size.y * ctx.scale * 0.01f;
    
    Matrix4 viewProj = ctx.camera->getProjectionMatrix(16.0f / 9.0f) * ctx.camera->getViewMatrix();
    
    shader->bind();
    shader->setMat4("uViewProjection", viewProj);
    shader->setVec3("uWorldPos", elementPos);
    shader->setVec3("uCameraRight", ctx.cameraRight);
    shader->setVec3("uCameraUp", ctx.cameraUp);
    shader->setFloat("uScale", 1.0f);
    
    uint32_t quadVao = 0, quadVbo = 0;
    glGenVertexArrays(1, &quadVao);
    glGenBuffers(1, &quadVbo);
    
    glBindVertexArray(quadVao);
    glBindBuffer(GL_ARRAY_BUFFER, quadVbo);
    glBufferData(GL_ARRAY_BUFFER, 6 * 2 * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    
    float halfW = scaledWidth * 0.5f;
    float halfH = scaledHeight * 0.5f;
    
    float bgVertices[6][2] = {
        {-halfW, -halfH}, {halfW, -halfH}, {halfW, halfH},
        {-halfW, -halfH}, {halfW, halfH}, {-halfW, halfH}
    };
    
    shader->setVec4("uColor", Vector4(bgColor));
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(bgVertices), bgVertices);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    float fillWidth = scaledWidth * glm::clamp(value, 0.0f, 1.0f);
    
    float fillVertices[6][2] = {
        {-halfW, -halfH}, {-halfW + fillWidth, -halfH}, {-halfW + fillWidth, halfH},
        {-halfW, -halfH}, {-halfW + fillWidth, halfH}, {-halfW, halfH}
    };
    
    shader->setVec4("uColor", Vector4(fillColor));
    glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(fillVertices), fillVertices);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    glBindVertexArray(0);
    glDeleteVertexArrays(1, &quadVao);
    glDeleteBuffers(1, &quadVbo);
    
    shader->unbind();
}

}  // namespace se
