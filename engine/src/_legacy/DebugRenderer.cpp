#include "engine/debug/DebugRenderer.h"
#include "engine/Application.h"
#include "engine/Camera.h"
#include "engine/Shader.h"
#include "engine/renderer/RenderCommand.h"
#include "engine/renderer/VertexArray.h"
#include "engine/ui/native/font/UIFont.h"
#include "engine/ui/native/font/UIFontManager.h"
#include "engine/Log.h"

#include <glad/glad.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>

namespace se {

DebugRenderer& DebugRenderer::Get() {
    static DebugRenderer instance;
    return instance;
}

DebugRenderer::DebugRenderer() {
    lines_.reserve(kInitialLineCapacity);
    persistentLines_.reserve(256);
    texts_.reserve(256);
    persistentTexts_.reserve(64);
}

void DebugRenderer::InitializeResources() {
    if (initialized_) return;

    const std::string vertexSrc = R"(#version 330 core
        layout (location = 0) in vec3 a_Position;
        layout (location = 1) in vec3 a_Color;
        
        uniform mat4 u_ViewProjection;
        
        out vec3 v_Color;
        
        void main() {
            gl_Position = u_ViewProjection * vec4(a_Position, 1.0);
            v_Color = a_Color;
        }
    )";

    const std::string fragmentSrc = R"(#version 330 core
        in vec3 v_Color;
        out vec4 FragColor;
        
        void main() {
            FragColor = vec4(v_Color, 1.0);
        }
    )";

    shader_ = std::make_shared<Shader>(vertexSrc, fragmentSrc);

    bufferCapacity_ = kInitialLineCapacity * 2 * 6 * sizeof(float);
    vertexBuffer_ = std::make_shared<VertexBuffer>(bufferCapacity_);
    
    BufferLayout layout = {
        {ShaderDataType::Float3, "a_Position"},
        {ShaderDataType::Float3, "a_Color"}
    };
    vertexBuffer_->SetLayout(layout);

    vertexArray_ = std::make_shared<VertexArray>();
    vertexArray_->AddVertexBuffer(vertexBuffer_);

    initialized_ = true;
    SE_LOG_DEBUG("[DebugRenderer] Initialized with capacity for {} lines", kInitialLineCapacity);
}

void DebugRenderer::InitializeTextResources() {
    if (textShader_) return;
    
    std::string vertPath = "assets/shaders/debug/debug_text.vert";
    std::string fragPath = "assets/shaders/debug/debug_text.frag";
    
    std::ifstream vertFile(vertPath);
    std::ifstream fragFile(fragPath);
    
    if (!vertFile.is_open() || !fragFile.is_open()) {
        SE_LOG_ERROR("[DebugRenderer] Failed to load text shader files");
        return;
    }
    
    std::stringstream vertStream, fragStream;
    vertStream << vertFile.rdbuf();
    fragStream << fragFile.rdbuf();
    
    textShader_ = std::make_shared<Shader>(vertStream.str(), fragStream.str());
    
    debugFont_ = ui::UIFontManager::Get().LoadFont("assets/fonts/Roboto-Regular.ttf", 12);
    
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
    
    SE_LOG_DEBUG("[DebugRenderer] Text rendering resources initialized");
}

void DebugRenderer::DrawText3D(const Vector3& worldPos, const std::string& text, 
                                const Vector3& color, float scale) {
    if (!enabled_ || text.empty()) return;
    std::lock_guard<std::mutex> lock(mutex_);
    texts_.push_back({worldPos, text, color, scale});
}

void DebugRenderer::DrawPersistentText3D(const Vector3& worldPos, const std::string& text, 
                                          const Vector3& color, float duration, float scale) {
    if (!enabled_ || text.empty()) return;
    std::lock_guard<std::mutex> lock(mutex_);
    persistentTexts_.push_back({worldPos, text, color, scale, duration});
}

void DebugRenderer::DrawLine(const Vector3& from, const Vector3& to, const Vector3& color) {
    if (!enabled_) return;
    std::lock_guard<std::mutex> lock(mutex_);
    lines_.push_back({from, to, color});
}

void DebugRenderer::DrawSphere(const Vector3& center, float radius, const Vector3& color, int segments) {
    if (!enabled_) return;
    
    const float pi = 3.14159265359f;
    
    for (int i = 0; i < segments; ++i) {
        float angle1 = (float)i / segments * 2.0f * pi;
        float angle2 = (float)(i + 1) / segments * 2.0f * pi;

        Vector3 p1 = center + Vector3(std::cos(angle1) * radius, 0, std::sin(angle1) * radius);
        Vector3 p2 = center + Vector3(std::cos(angle2) * radius, 0, std::sin(angle2) * radius);
        DrawLine(p1, p2, color);

        p1 = center + Vector3(std::cos(angle1) * radius, std::sin(angle1) * radius, 0);
        p2 = center + Vector3(std::cos(angle2) * radius, std::sin(angle2) * radius, 0);
        DrawLine(p1, p2, color);

        p1 = center + Vector3(0, std::sin(angle1) * radius, std::cos(angle1) * radius);
        p2 = center + Vector3(0, std::sin(angle2) * radius, std::cos(angle2) * radius);
        DrawLine(p1, p2, color);
    }
}

void DebugRenderer::DrawBox(const Vector3& min, const Vector3& max, const Vector3& color) {
    if (!enabled_) return;
    
    DrawLine({min.x, min.y, min.z}, {max.x, min.y, min.z}, color);
    DrawLine({max.x, min.y, min.z}, {max.x, min.y, max.z}, color);
    DrawLine({max.x, min.y, max.z}, {min.x, min.y, max.z}, color);
    DrawLine({min.x, min.y, max.z}, {min.x, min.y, min.z}, color);
    
    DrawLine({min.x, max.y, min.z}, {max.x, max.y, min.z}, color);
    DrawLine({max.x, max.y, min.z}, {max.x, max.y, max.z}, color);
    DrawLine({max.x, max.y, max.z}, {min.x, max.y, max.z}, color);
    DrawLine({min.x, max.y, max.z}, {min.x, max.y, min.z}, color);
    
    DrawLine({min.x, min.y, min.z}, {min.x, max.y, min.z}, color);
    DrawLine({max.x, min.y, min.z}, {max.x, max.y, min.z}, color);
    DrawLine({max.x, min.y, max.z}, {max.x, max.y, max.z}, color);
    DrawLine({min.x, min.y, max.z}, {min.x, max.y, max.z}, color);
}

void DebugRenderer::DrawPoint(const Vector3& pos, float size, const Vector3& color) {
    if (!enabled_) return;
    
    float half = size * 0.5f;
    DrawLine(pos - Vector3(half, 0, 0), pos + Vector3(half, 0, 0), color);
    DrawLine(pos - Vector3(0, half, 0), pos + Vector3(0, half, 0), color);
    DrawLine(pos - Vector3(0, 0, half), pos + Vector3(0, 0, half), color);
}

void DebugRenderer::DrawArrow(const Vector3& from, const Vector3& to, const Vector3& color, float headSize) {
    if (!enabled_) return;
    
    DrawLine(from, to, color);
    
    Vector3 dir = glm::normalize(to - from);
    Vector3 right = glm::normalize(glm::cross(dir, Vector3(0, 1, 0)));
    if (glm::length(right) < 0.001f) {
        right = glm::normalize(glm::cross(dir, Vector3(1, 0, 0)));
    }
    Vector3 up = glm::normalize(glm::cross(right, dir));
    
    Vector3 headBase = to - dir * headSize;
    DrawLine(to, headBase + right * headSize * 0.5f, color);
    DrawLine(to, headBase - right * headSize * 0.5f, color);
    DrawLine(to, headBase + up * headSize * 0.5f, color);
    DrawLine(to, headBase - up * headSize * 0.5f, color);
}

void DebugRenderer::DrawCircle(const Vector3& center, float radius, const Vector3& color,
                                const Vector3& normal, int segments) {
    if (!enabled_) return;
    
    Vector3 right = glm::normalize(glm::cross(normal, Vector3(0, 1, 0)));
    if (glm::length(right) < 0.001f) {
        right = glm::normalize(glm::cross(normal, Vector3(1, 0, 0)));
    }
    Vector3 forward = glm::normalize(glm::cross(right, normal));
    
    const float pi = 3.14159265359f;
    for (int i = 0; i < segments; ++i) {
        float angle1 = (float)i / segments * 2.0f * pi;
        float angle2 = (float)(i + 1) / segments * 2.0f * pi;
        
        Vector3 p1 = center + right * std::cos(angle1) * radius + forward * std::sin(angle1) * radius;
        Vector3 p2 = center + right * std::cos(angle2) * radius + forward * std::sin(angle2) * radius;
        DrawLine(p1, p2, color);
    }
}

void DebugRenderer::DrawPath(const std::vector<Vector3>& waypoints, const Vector3& color, float waypointSize) {
    if (!enabled_ || waypoints.empty()) return;
    
    for (size_t i = 0; i < waypoints.size() - 1; ++i) {
        DrawLine(waypoints[i], waypoints[i + 1], color);
    }
    
    for (const auto& wp : waypoints) {
        DrawSphere(wp, waypointSize, color, 6);
    }
}

void DebugRenderer::DrawSquare(const Vector3& center, float halfSize, const Vector3& color,
                                const Vector3& normal) {
    if (!enabled_) return;
    
    Vector3 right = glm::normalize(glm::cross(normal, Vector3(0, 0, 1)));
    if (glm::length(right) < 0.001f) {
        right = glm::normalize(glm::cross(normal, Vector3(1, 0, 0)));
    }
    Vector3 forward = glm::normalize(glm::cross(right, normal));
    
    Vector3 corners[4] = {
        center + right * halfSize + forward * halfSize,
        center - right * halfSize + forward * halfSize,
        center - right * halfSize - forward * halfSize,
        center + right * halfSize - forward * halfSize
    };
    
    DrawLine(corners[0], corners[1], color);
    DrawLine(corners[1], corners[2], color);
    DrawLine(corners[2], corners[3], color);
    DrawLine(corners[3], corners[0], color);
}

void DebugRenderer::DrawPersistentLine(const Vector3& from, const Vector3& to, 
                                        const Vector3& color, float duration) {
    if (!enabled_) return;
    std::lock_guard<std::mutex> lock(mutex_);
    persistentLines_.push_back({from, to, color, duration});
}

void DebugRenderer::DrawPersistentSphere(const Vector3& center, float radius, 
                                          const Vector3& color, float duration, int segments) {
    if (!enabled_) return;
    
    const float pi = 3.14159265359f;
    
    for (int i = 0; i < segments; ++i) {
        float angle1 = (float)i / segments * 2.0f * pi;
        float angle2 = (float)(i + 1) / segments * 2.0f * pi;

        Vector3 p1 = center + Vector3(std::cos(angle1) * radius, 0, std::sin(angle1) * radius);
        Vector3 p2 = center + Vector3(std::cos(angle2) * radius, 0, std::sin(angle2) * radius);
        DrawPersistentLine(p1, p2, color, duration);

        p1 = center + Vector3(std::cos(angle1) * radius, std::sin(angle1) * radius, 0);
        p2 = center + Vector3(std::cos(angle2) * radius, std::sin(angle2) * radius, 0);
        DrawPersistentLine(p1, p2, color, duration);

        p1 = center + Vector3(0, std::sin(angle1) * radius, std::cos(angle1) * radius);
        p2 = center + Vector3(0, std::sin(angle2) * radius, std::cos(angle2) * radius);
        DrawPersistentLine(p1, p2, color, duration);
    }
}

void DebugRenderer::DrawPersistentPoint(const Vector3& pos, float size, 
                                         const Vector3& color, float duration) {
    if (!enabled_) return;
    
    float half = size * 0.5f;
    DrawPersistentLine(pos - Vector3(half, 0, 0), pos + Vector3(half, 0, 0), color, duration);
    DrawPersistentLine(pos - Vector3(0, half, 0), pos + Vector3(0, half, 0), color, duration);
    DrawPersistentLine(pos - Vector3(0, 0, half), pos + Vector3(0, 0, half), color, duration);
}

void DebugRenderer::Update(float deltaTime) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    persistentLines_.erase(
        std::remove_if(persistentLines_.begin(), persistentLines_.end(),
            [deltaTime](PersistentLine& line) {
                line.remainingTime -= deltaTime;
                return line.remainingTime <= 0.0f;
            }),
        persistentLines_.end()
    );
    
    persistentTexts_.erase(
        std::remove_if(persistentTexts_.begin(), persistentTexts_.end(),
            [deltaTime](PersistentText& text) {
                text.remainingTime -= deltaTime;
                return text.remainingTime <= 0.0f;
            }),
        persistentTexts_.end()
    );
}

void DebugRenderer::Flush(const Camera& camera) {
    if (!enabled_) {
        lines_.clear();
        texts_.clear();
        return;
    }
    
    InitializeResources();
    
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& pLine : persistentLines_) {
            lines_.push_back({pLine.from, pLine.to, pLine.color});
        }
        for (const auto& pText : persistentTexts_) {
            texts_.push_back({pText.position, pText.text, pText.color, pText.scale});
        }
    }
    
    if (!lines_.empty()) {
        std::vector<float> vertices;
        vertices.reserve(lines_.size() * 12);
        
        for (const auto& line : lines_) {
            vertices.push_back(line.from.x);
            vertices.push_back(line.from.y);
            vertices.push_back(line.from.z);
            vertices.push_back(line.color.x);
            vertices.push_back(line.color.y);
            vertices.push_back(line.color.z);
            
            vertices.push_back(line.to.x);
            vertices.push_back(line.to.y);
            vertices.push_back(line.to.z);
            vertices.push_back(line.color.x);
            vertices.push_back(line.color.y);
            vertices.push_back(line.color.z);
        }
        
        const uint32_t requiredSize = static_cast<uint32_t>(vertices.size() * sizeof(float));
        
        if (requiredSize > bufferCapacity_) {
            bufferCapacity_ = static_cast<uint32_t>(requiredSize * 1.5f);
            vertexBuffer_ = std::make_shared<VertexBuffer>(bufferCapacity_);
            
            BufferLayout layout = {
                {ShaderDataType::Float3, "a_Position"},
                {ShaderDataType::Float3, "a_Color"}
            };
            vertexBuffer_->SetLayout(layout);
            
            vertexArray_ = std::make_shared<VertexArray>();
            vertexArray_->AddVertexBuffer(vertexBuffer_);
        }
        
        vertexBuffer_->SetData(vertices.data(), requiredSize);
        
        if (shader_) {
            shader_->bind();
            
            auto& window = Application::Get().GetWindow();
            float aspectRatio = (float)window.GetWidth() / (float)window.GetHeight();
            
            glm::mat4 view = camera.getViewMatrix();
            glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);
            glm::mat4 viewProjection = projection * view;
            
            shader_->setMat4("u_ViewProjection", viewProjection);
        }
        
        glLineWidth(1.0f);
        RenderCommand::DrawLines(vertexArray_.get(), static_cast<uint32_t>(lines_.size() * 2));
        
        if (shader_) {
            shader_->unbind();
        }
    }
    
    if (!texts_.empty()) {
        RenderTexts(camera);
    }
    
    lines_.clear();
    texts_.clear();
}

void DebugRenderer::RenderTexts(const Camera& camera) {
    InitializeTextResources();
    
    if (!textShader_ || !debugFont_ || !debugFont_->IsLoaded()) return;
    
    auto& window = Application::Get().GetWindow();
    float aspectRatio = (float)window.GetWidth() / (float)window.GetHeight();
    
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);
    glm::mat4 viewProjection = projection * view;
    
    glm::vec3 camRight = glm::vec3(view[0][0], view[1][0], view[2][0]);
    glm::vec3 camUp = glm::vec3(view[0][1], view[1][1], view[2][1]);
    
    textShader_->bind();
    textShader_->setMat4("uViewProjection", viewProjection);
    textShader_->setVec3("uCameraRight", camRight);
    textShader_->setVec3("uCameraUp", camUp);
    textShader_->setInt("uFontAtlas", 0);
    
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, debugFont_->GetAtlasTextureId());
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    
    glBindVertexArray(textVao_);
    
    for (const auto& textCmd : texts_) {
        textShader_->setVec3("uWorldPos", textCmd.position);
        textShader_->setVec3("uColor", textCmd.color);
        textShader_->setFloat("uScale", textCmd.scale * 0.01f);
        
        float cursorX = 0.0f;
        
        for (char c : textCmd.text) {
            const auto& glyph = debugFont_->GetGlyph(static_cast<uint32_t>(c));
            
            float xpos = cursorX + glyph.bearingX;
            float ypos = -(glyph.height - glyph.bearingY);
            float w = glyph.width;
            float h = glyph.height;
            
            float vertices[6][4] = {
                {xpos,     ypos + h,   glyph.u0, glyph.v0},
                {xpos,     ypos,       glyph.u0, glyph.v1},
                {xpos + w, ypos,       glyph.u1, glyph.v1},
                
                {xpos,     ypos + h,   glyph.u0, glyph.v0},
                {xpos + w, ypos,       glyph.u1, glyph.v1},
                {xpos + w, ypos + h,   glyph.u1, glyph.v0}
            };
            
            glBindBuffer(GL_ARRAY_BUFFER, textVbo_);
            glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices);
            
            glDrawArrays(GL_TRIANGLES, 0, 6);
            
            cursorX += glyph.advance;
        }
    }
    
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    
    textShader_->unbind();
}

void DebugRenderer::Clear() {
    std::lock_guard<std::mutex> lock(mutex_);
    lines_.clear();
    texts_.clear();
}

}  // namespace se
