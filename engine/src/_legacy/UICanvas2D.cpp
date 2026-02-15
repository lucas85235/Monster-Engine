#include "engine/ui/native/render/UICanvas2D.h"
#include "engine/ui/native/font/UIFontManager.h"
#include "engine/ui/native/font/UIFont.h"
#include "engine/Log.h"

#include <glad/glad.h>
#include <algorithm>

namespace se::ui {

namespace {

// Simple 2D shader for UI rendering
const char* kVertexShader = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
layout(location = 2) in vec4 aColor;

out vec2 vUV;
out vec4 vColor;

uniform mat4 uProjection;

void main() {
    gl_Position = uProjection * vec4(aPos, 0.0, 1.0);
    vUV = aUV;
    vColor = aColor;
}
)";

const char* kFragmentShader = R"(
#version 330 core
in vec2 vUV;
in vec4 vColor;

out vec4 FragColor;

uniform sampler2D uTexture;
uniform int uUseTexture;
uniform int uIsText;

void main() {
    if (uIsText == 1) {
        // Text rendering: red channel is glyph alpha
        float alpha = texture(uTexture, vUV).r;
        FragColor = vec4(vColor.rgb, vColor.a * alpha);
    } else if (uUseTexture == 1) {
        FragColor = texture(uTexture, vUV) * vColor;
    } else {
        FragColor = vColor;
    }
}
)";

struct Vertex2D {
    glm::vec2 position;
    glm::vec2 uv;
    glm::vec4 color;
};

}  // namespace

UICanvas2D& UICanvas2D::Get() {
    static UICanvas2D instance;
    return instance;
}

UICanvas2D::UICanvas2D() {
    transformStack_.push_back(glm::mat3(1.0f));
}

UICanvas2D::~UICanvas2D() {
    ShutdownResources();
}

void UICanvas2D::InitializeResources() {
    if (initialized_) return;
    
    // Create VAO
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);
    
    glBindVertexArray(vao_);
    
    // Allocate VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex2D) * 4096, nullptr, GL_DYNAMIC_DRAW);
    
    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, position));
    
    // UV
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, uv));
    
    // Color
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex2D), (void*)offsetof(Vertex2D, color));
    
    // EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * 6144, nullptr, GL_DYNAMIC_DRAW);
    
    glBindVertexArray(0);
    
    // Compile shaders
    auto compileShader = [](const char* source, GLenum type) -> uint32_t {
        uint32_t shader = glCreateShader(type);
        glShaderSource(shader, 1, &source, nullptr);
        glCompileShader(shader);
        
        int success;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char log[512];
            glGetShaderInfoLog(shader, 512, nullptr, log);
            SE_LOG_ERROR("UICanvas2D shader compile error: {}", log);
        }
        return shader;
    };
    
    uint32_t vs = compileShader(kVertexShader, GL_VERTEX_SHADER);
    uint32_t fs = compileShader(kFragmentShader, GL_FRAGMENT_SHADER);
    
    shaderProgram_ = glCreateProgram();
    glAttachShader(shaderProgram_, vs);
    glAttachShader(shaderProgram_, fs);
    glLinkProgram(shaderProgram_);
    
    int success;
    glGetProgramiv(shaderProgram_, GL_LINK_STATUS, &success);
    if (!success) {
        char log[512];
        glGetProgramInfoLog(shaderProgram_, 512, nullptr, log);
        SE_LOG_ERROR("UICanvas2D shader link error: {}", log);
    }
    
    glDeleteShader(vs);
    glDeleteShader(fs);
    
    initialized_ = true;
    SE_LOG_INFO("UICanvas2D resources initialized");
}

void UICanvas2D::ShutdownResources() {
    if (!initialized_) return;
    
    glDeleteProgram(shaderProgram_);
    glDeleteBuffers(1, &vbo_);
    glDeleteBuffers(1, &ebo_);
    glDeleteVertexArrays(1, &vao_);
    
    shaderProgram_ = 0;
    vbo_ = 0;
    ebo_ = 0;
    vao_ = 0;
    
    initialized_ = false;
    SE_LOG_INFO("UICanvas2D resources shutdown");
}

void UICanvas2D::BeginFrame() {
    if (!initialized_) {
        InitializeResources();
    }
    
    // Only clear main commands, NOT overlays
    if (!isInOverlayMode_) {
        commands_.clear();
    }
    currentZIndex_ = 0;
    
    // Reset transform stack
    transformStack_.clear();
    transformStack_.push_back(glm::mat3(1.0f));
    
    // Reset scissor stack
    scissorStack_.clear();
}

void UICanvas2D::EndFrame() {
    if (isInOverlayMode_) {
        // Cache overlay commands separately
        cachedOverlayCommands_ = overlayCommands_;
    } else {
        // Sort and cache main commands
        SortCommands();
        cachedCommands_ = commands_;
    }
    isDirty_ = false;
}

void UICanvas2D::BeginOverlay() {
    if (!initialized_) {
        InitializeResources();
    }
    
    isInOverlayMode_ = true;
    overlayCommands_.clear();
    currentZIndex_ = 10000;  // High Z-index for overlays
    
    // Reset transform stack
    transformStack_.clear();
    transformStack_.push_back(glm::mat3(1.0f));
    
    // Reset scissor stack
    scissorStack_.clear();
}

void UICanvas2D::EndOverlay() {
    // Cache overlay commands
    cachedOverlayCommands_ = overlayCommands_;
    isInOverlayMode_ = false;
}

void UICanvas2D::ClearOverlays() {
    overlayCommands_.clear();
    cachedOverlayCommands_.clear();
}

void UICanvas2D::Render() {
    // Skip if nothing to render
    if (cachedCommands_.empty() && cachedOverlayCommands_.empty()) return;
    
    // Setup OpenGL state
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    
    glUseProgram(shaderProgram_);
    
    // Set orthographic projection
    glm::mat4 projection = glm::ortho(0.0f, viewportSize_.x, viewportSize_.y, 0.0f, -1.0f, 1.0f);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "uProjection"), 1, GL_FALSE, &projection[0][0]);
    
    glBindVertexArray(vao_);
    
    // Render main UI commands first
    RenderCommands();
    
    // Render overlay commands on top (always last)
    if (!cachedOverlayCommands_.empty()) {
        auto tempCached = cachedCommands_;
        cachedCommands_ = cachedOverlayCommands_;
        RenderCommands();
        cachedCommands_ = tempCached;
    }
    
    glBindVertexArray(0);
    glUseProgram(0);
    
    // Restore state
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_SCISSOR_TEST);
}

void UICanvas2D::RenderCommands() {
    std::vector<Vertex2D> vertices;
    std::vector<uint32_t> indices;
    bool currentScissorEnabled = false;
    
    int useTextureLoc = glGetUniformLocation(shaderProgram_, "uUseTexture");
    
    // Render from cached commands (retained mode)
    for (const auto& cmd : cachedCommands_) {
        switch (cmd.type) {
            case DrawCommandType::RECT_FILLED: {
                glUniform1i(useTextureLoc, 0);
                
                const auto& r = cmd.rectFilled;
                uint32_t baseIdx = static_cast<uint32_t>(vertices.size());
                
                // Apply transform to corners
                glm::vec2 p0{r.rect.x, r.rect.y};
                glm::vec2 p1{r.rect.x + r.rect.z, r.rect.y};
                glm::vec2 p2{r.rect.x + r.rect.z, r.rect.y + r.rect.w};
                glm::vec2 p3{r.rect.x, r.rect.y + r.rect.w};
                
                vertices.push_back({p0, {0, 0}, r.color});
                vertices.push_back({p1, {1, 0}, r.color});
                vertices.push_back({p2, {1, 1}, r.color});
                vertices.push_back({p3, {0, 1}, r.color});
                
                indices.push_back(baseIdx + 0);
                indices.push_back(baseIdx + 1);
                indices.push_back(baseIdx + 2);
                indices.push_back(baseIdx + 0);
                indices.push_back(baseIdx + 2);
                indices.push_back(baseIdx + 3);
                break;
            }
            
            case DrawCommandType::TEXTURE: {
                FlushBatch();
                
                glUniform1i(useTextureLoc, 1);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, cmd.texture.textureId);
                glUniform1i(glGetUniformLocation(shaderProgram_, "uTexture"), 0);
                
                const auto& t = cmd.texture;
                uint32_t baseIdx = static_cast<uint32_t>(vertices.size());
                
                glm::vec2 p0{t.rect.x, t.rect.y};
                glm::vec2 p1{t.rect.x + t.rect.z, t.rect.y};
                glm::vec2 p2{t.rect.x + t.rect.z, t.rect.y + t.rect.w};
                glm::vec2 p3{t.rect.x, t.rect.y + t.rect.w};
                
                vertices.push_back({p0, {t.uvRect.x, t.uvRect.y}, t.modulate});
                vertices.push_back({p1, {t.uvRect.z, t.uvRect.y}, t.modulate});
                vertices.push_back({p2, {t.uvRect.z, t.uvRect.w}, t.modulate});
                vertices.push_back({p3, {t.uvRect.x, t.uvRect.w}, t.modulate});
                
                indices.push_back(baseIdx + 0);
                indices.push_back(baseIdx + 1);
                indices.push_back(baseIdx + 2);
                indices.push_back(baseIdx + 0);
                indices.push_back(baseIdx + 2);
                indices.push_back(baseIdx + 3);
                break;
            }
            
            case DrawCommandType::SCISSOR_PUSH: {
                FlushBatch();
                
                glEnable(GL_SCISSOR_TEST);
                const auto& s = cmd.scissor;
                glScissor(
                    static_cast<int>(s.rect.x),
                    static_cast<int>(viewportSize_.y - s.rect.y - s.rect.w),
                    static_cast<int>(s.rect.z),
                    static_cast<int>(s.rect.w)
                );
                currentScissorEnabled = true;
                break;
            }
            
            case DrawCommandType::SCISSOR_POP: {
                FlushBatch();
                
                if (scissorStack_.empty()) {
                    glDisable(GL_SCISSOR_TEST);
                    currentScissorEnabled = false;
                }
                break;
            }
            
            case DrawCommandType::LINE: {
                glUniform1i(useTextureLoc, 0);
                
                const auto& l = cmd.line;
                uint32_t baseIdx = static_cast<uint32_t>(vertices.size());
                
                // Calculate perpendicular direction for line width
                glm::vec2 dir = l.to - l.from;
                float len = glm::length(dir);
                if (len > 0.0001f) {
                    dir /= len;
                }
                glm::vec2 perp = glm::vec2(-dir.y, dir.x) * (l.width * 0.5f);
                
                glm::vec2 p0 = l.from - perp;
                glm::vec2 p1 = l.from + perp;
                glm::vec2 p2 = l.to + perp;
                glm::vec2 p3 = l.to - perp;
                
                vertices.push_back({p0, {0, 0}, l.color});
                vertices.push_back({p1, {1, 0}, l.color});
                vertices.push_back({p2, {1, 1}, l.color});
                vertices.push_back({p3, {0, 1}, l.color});
                
                indices.push_back(baseIdx + 0);
                indices.push_back(baseIdx + 1);
                indices.push_back(baseIdx + 2);
                indices.push_back(baseIdx + 0);
                indices.push_back(baseIdx + 2);
                indices.push_back(baseIdx + 3);
                break;
            }
            
            case DrawCommandType::TEXT: {
                // Flush previous geometry
                if (!vertices.empty()) {
                    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex2D), vertices.data());
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
                    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(uint32_t), indices.data());
                    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);
                    vertices.clear();
                    indices.clear();
                }
                
                // Get font (skip if not initialized)
                auto& fontMgr = UIFontManager::Get();
                if (!fontMgr.IsInitialized()) break;
                
                auto font = fontMgr.GetDefaultFont(cmd.textFontSize);
                if (!font) break;
                
                // Enable text mode
                glUniform1i(useTextureLoc, 1);
                glUniform1i(glGetUniformLocation(shaderProgram_, "uIsText"), 1);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, font->GetAtlasTextureId());
                glUniform1i(glGetUniformLocation(shaderProgram_, "uTexture"), 0);
                
                // Build glyph quads
                float cursorX = cmd.textPosition.x;
                float cursorY = cmd.textPosition.y + font->GetAscender();
                
                for (char c : cmd.textContent) {
                    const UIGlyph& glyph = font->GetGlyph(static_cast<uint32_t>(c));
                    if (!glyph.valid) {
                        cursorX += glyph.advance;
                        continue;
                    }
                    
                    float x = cursorX + glyph.bearingX;
                    float y = cursorY - glyph.bearingY;
                    float w = static_cast<float>(glyph.width);
                    float h = static_cast<float>(glyph.height);
                    
                    uint32_t baseIdx = static_cast<uint32_t>(vertices.size());
                    
                    vertices.push_back({{x, y}, {glyph.u0, glyph.v0}, cmd.textColor});
                    vertices.push_back({{x + w, y}, {glyph.u1, glyph.v0}, cmd.textColor});
                    vertices.push_back({{x + w, y + h}, {glyph.u1, glyph.v1}, cmd.textColor});
                    vertices.push_back({{x, y + h}, {glyph.u0, glyph.v1}, cmd.textColor});
                    
                    indices.push_back(baseIdx + 0);
                    indices.push_back(baseIdx + 1);
                    indices.push_back(baseIdx + 2);
                    indices.push_back(baseIdx + 0);
                    indices.push_back(baseIdx + 2);
                    indices.push_back(baseIdx + 3);
                    
                    cursorX += glyph.advance;
                }
                
                // Flush text batch
                if (!vertices.empty()) {
                    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
                    glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex2D), vertices.data());
                    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
                    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(uint32_t), indices.data());
                    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);
                    vertices.clear();
                    indices.clear();
                }
                
                // Disable text mode
                glUniform1i(glGetUniformLocation(shaderProgram_, "uIsText"), 0);
                break;
            }
            
            default:
                break;
        }
    }
    
    // Flush remaining geometry
    if (!vertices.empty()) {
        glBindBuffer(GL_ARRAY_BUFFER, vbo_);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(Vertex2D), vertices.data());
        
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
        glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(uint32_t), indices.data());
        
        glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices.size()), GL_UNSIGNED_INT, nullptr);
    }
}

void UICanvas2D::FlushBatch() {
    // Batch flushing logic for state changes
}

void UICanvas2D::SortCommands() {
    std::stable_sort(commands_.begin(), commands_.end(), 
        [](const DrawCommand& a, const DrawCommand& b) {
            return a.zIndex < b.zIndex;
        });
}

void UICanvas2D::PushTransform(const glm::mat3& transform) {
    glm::mat3 combined = transformStack_.back() * transform;
    transformStack_.push_back(combined);
}

void UICanvas2D::PopTransform() {
    if (transformStack_.size() > 1) {
        transformStack_.pop_back();
    }
}

glm::mat3 UICanvas2D::GetCurrentTransform() const {
    return transformStack_.back();
}

void UICanvas2D::DrawRect(const glm::vec4& rect, const glm::vec4& color, float lineWidth) {
    // Draw 4 lines forming a rectangle outline
    DrawLine({rect.x, rect.y}, {rect.x + rect.z, rect.y}, color, lineWidth);
    DrawLine({rect.x + rect.z, rect.y}, {rect.x + rect.z, rect.y + rect.w}, color, lineWidth);
    DrawLine({rect.x + rect.z, rect.y + rect.w}, {rect.x, rect.y + rect.w}, color, lineWidth);
    DrawLine({rect.x, rect.y + rect.w}, {rect.x, rect.y}, color, lineWidth);
}

void UICanvas2D::DrawRectFilled(const glm::vec4& rect, const glm::vec4& color, float cornerRadius) {
    DrawCommand cmd;
    cmd.type = DrawCommandType::RECT_FILLED;
    cmd.zIndex = currentZIndex_;
    cmd.transform = GetCurrentTransform();
    cmd.rectFilled.rect = rect;
    cmd.rectFilled.color = color;
    cmd.rectFilled.cornerRadius = cornerRadius;
    
    if (isInOverlayMode_) {
        overlayCommands_.push_back(std::move(cmd));
    } else {
        commands_.push_back(std::move(cmd));
    }
}

void UICanvas2D::DrawTexture(uint32_t textureId, const glm::vec4& rect, const glm::vec4& uvRect, const glm::vec4& modulate) {
    DrawCommand cmd;
    cmd.type = DrawCommandType::TEXTURE;
    cmd.zIndex = currentZIndex_;
    cmd.transform = GetCurrentTransform();
    cmd.texture.textureId = textureId;
    cmd.texture.rect = rect;
    cmd.texture.uvRect = uvRect;
    cmd.texture.modulate = modulate;
    
    if (isInOverlayMode_) {
        overlayCommands_.push_back(std::move(cmd));
    } else {
        commands_.push_back(std::move(cmd));
    }
}

void UICanvas2D::DrawNinePatch(uint32_t textureId, const glm::vec4& rect, const glm::vec4& sourceRect, const glm::vec4& margins, const glm::vec4& modulate) {
    DrawCommand cmd;
    cmd.type = DrawCommandType::NINE_PATCH;
    cmd.zIndex = currentZIndex_;
    cmd.transform = GetCurrentTransform();
    cmd.ninePatch.textureId = textureId;
    cmd.ninePatch.rect = rect;
    cmd.ninePatch.sourceRect = sourceRect;
    cmd.ninePatch.margins = margins;
    cmd.ninePatch.modulate = modulate;
    
    if (isInOverlayMode_) {
        overlayCommands_.push_back(std::move(cmd));
    } else {
        commands_.push_back(std::move(cmd));
    }
}

void UICanvas2D::DrawText(const std::string& text, const glm::vec2& position, const glm::vec4& color, uint32_t fontId, float fontSize) {
    DrawCommand cmd;
    cmd.type = DrawCommandType::TEXT;
    cmd.zIndex = currentZIndex_;
    cmd.transform = GetCurrentTransform();
    cmd.textContent = text;
    cmd.textPosition = position;
    cmd.textColor = color;
    cmd.textFontId = fontId;
    cmd.textFontSize = fontSize;
    
    if (isInOverlayMode_) {
        overlayCommands_.push_back(std::move(cmd));
    } else {
        commands_.push_back(std::move(cmd));
    }
}

void UICanvas2D::DrawLine(const glm::vec2& from, const glm::vec2& to, const glm::vec4& color, float width) {
    DrawCommand cmd;
    cmd.type = DrawCommandType::LINE;
    cmd.zIndex = currentZIndex_;
    cmd.transform = GetCurrentTransform();
    cmd.line.from = from;
    cmd.line.to = to;
    cmd.line.color = color;
    cmd.line.width = width;
    
    if (isInOverlayMode_) {
        overlayCommands_.push_back(std::move(cmd));
    } else {
        commands_.push_back(std::move(cmd));
    }
}

void UICanvas2D::DrawCircle(const glm::vec2& center, float radius, const glm::vec4& color, bool filled) {
    DrawCommand cmd;
    cmd.type = DrawCommandType::CIRCLE;
    cmd.zIndex = currentZIndex_;
    cmd.transform = GetCurrentTransform();
    cmd.circle.center = center;
    cmd.circle.radius = radius;
    cmd.circle.color = color;
    cmd.circle.filled = filled;
    
    if (isInOverlayMode_) {
        overlayCommands_.push_back(std::move(cmd));
    } else {
        commands_.push_back(std::move(cmd));
    }
}

void UICanvas2D::PushScissor(const glm::vec4& rect) {
    scissorStack_.push_back(rect);
    
    DrawCommand cmd;
    cmd.type = DrawCommandType::SCISSOR_PUSH;
    cmd.scissor.rect = rect;
    
    if (isInOverlayMode_) {
        overlayCommands_.push_back(std::move(cmd));
    } else {
        commands_.push_back(std::move(cmd));
    }
}

void UICanvas2D::PopScissor() {
    if (!scissorStack_.empty()) {
        scissorStack_.pop_back();
    }
    
    DrawCommand cmd;
    cmd.type = DrawCommandType::SCISSOR_POP;
    
    if (isInOverlayMode_) {
        overlayCommands_.push_back(std::move(cmd));
    } else {
        commands_.push_back(std::move(cmd));
    }
}

void UICanvas2D::SetViewport(float width, float height) {
    viewportSize_ = {width, height};
}

}  // namespace se::ui
