#pragma once

#include <se_pch.h>
#include <cstdint>
#include <vector>
#include <memory>
#include <functional>

namespace se::ui {

// Draw command types
enum class DrawCommandType : uint8_t {
    RECT,
    RECT_FILLED,
    TEXTURE,
    TEXTURE_RECT,
    NINE_PATCH,
    TEXT,
    LINE,
    CIRCLE,
    POLYGON,
    SCISSOR_PUSH,
    SCISSOR_POP
};

// Draw command data structures
struct DrawRect {
    glm::vec4 rect;  // x, y, width, height
    glm::vec4 color;
    float lineWidth = 1.0f;
};

struct DrawRectFilled {
    glm::vec4 rect;
    glm::vec4 color;
    float cornerRadius = 0.0f;
};

struct DrawTexture {
    uint32_t textureId;
    glm::vec4 rect;
    glm::vec4 uvRect;  // u0, v0, u1, v1
    glm::vec4 modulate;
};

struct DrawNinePatch {
    uint32_t textureId;
    glm::vec4 rect;
    glm::vec4 sourceRect;
    glm::vec4 margins;  // left, top, right, bottom
    glm::vec4 modulate;
};

struct DrawText {
    std::string text;
    glm::vec2 position;
    glm::vec4 color;
    uint32_t fontId;
    float fontSize;
};

struct DrawLine {
    glm::vec2 from;
    glm::vec2 to;
    glm::vec4 color;
    float width;
};

struct DrawCircle {
    glm::vec2 center;
    float radius;
    glm::vec4 color;
    bool filled;
};

struct DrawScissor {
    glm::vec4 rect;
};

// Unified draw command
struct DrawCommand {
    DrawCommandType type;
    int32_t zIndex = 0;
    glm::mat3 transform{1.0f};
    
    union {
        DrawRect rect;
        DrawRectFilled rectFilled;
        DrawTexture texture;
        DrawNinePatch ninePatch;
        DrawLine line;
        DrawCircle circle;
        DrawScissor scissor;
    };
    
    // Text needs separate storage due to std::string
    std::string textContent;
    glm::vec2 textPosition;
    glm::vec4 textColor;
    uint32_t textFontId;
    float textFontSize;
    
    DrawCommand() : type(DrawCommandType::RECT), rectFilled{} {}
    ~DrawCommand() = default;
};

/**
 * @class UICanvas2D
 * @brief Retained-mode 2D rendering system for UI
 *
 * Collects draw commands from UI controls and renders them
 * in order using OpenGL batch rendering.
 */
class UICanvas2D {
public:
    static UICanvas2D& Get();
    
    // Frame management
    void BeginFrame();
    void EndFrame();
    void Render();
    
    // Overlay management - overlays are rendered on top and not cleared by BeginFrame
    void BeginOverlay();
    void EndOverlay();
    void ClearOverlays();
    
    // Retained mode control
    void MarkDirty() { isDirty_ = true; }
    bool IsDirty() const { return isDirty_; }
    bool HasCachedCommands() const { return !cachedCommands_.empty(); }
    
    // Transform stack
    void PushTransform(const glm::mat3& transform);
    void PopTransform();
    glm::mat3 GetCurrentTransform() const;
    
    // Drawing commands
    void DrawRect(const glm::vec4& rect, const glm::vec4& color, float lineWidth = 1.0f);
    void DrawRectFilled(const glm::vec4& rect, const glm::vec4& color, float cornerRadius = 0.0f);
    void DrawTexture(uint32_t textureId, const glm::vec4& rect, const glm::vec4& uvRect = {0, 0, 1, 1}, const glm::vec4& modulate = {1, 1, 1, 1});
    void DrawNinePatch(uint32_t textureId, const glm::vec4& rect, const glm::vec4& sourceRect, const glm::vec4& margins, const glm::vec4& modulate = {1, 1, 1, 1});
    void DrawText(const std::string& text, const glm::vec2& position, const glm::vec4& color, uint32_t fontId = 0, float fontSize = 16.0f);
    void DrawLine(const glm::vec2& from, const glm::vec2& to, const glm::vec4& color, float width = 1.0f);
    void DrawCircle(const glm::vec2& center, float radius, const glm::vec4& color, bool filled = true);
    
    // Scissor (clipping)
    void PushScissor(const glm::vec4& rect);
    void PopScissor();
    
    // Z-ordering
    void SetZIndex(int32_t z) { currentZIndex_ = z; }
    int32_t GetZIndex() const { return currentZIndex_; }
    
    // Viewport
    void SetViewport(float width, float height);
    glm::vec2 GetViewportSize() const { return viewportSize_; }
    
private:
    UICanvas2D();
    ~UICanvas2D();
    
    void InitializeResources();
    void ShutdownResources();
    void FlushBatch();
    void RenderCommands();
    
    // Sorted rendering
    void SortCommands();
    
private:
    std::vector<DrawCommand> commands_;
    std::vector<DrawCommand> cachedCommands_;  // Persisted commands for retained mode
    std::vector<DrawCommand> overlayCommands_;  // Overlay commands - never cleared by BeginFrame
    std::vector<DrawCommand> cachedOverlayCommands_;  // Cached overlays for rendering
    std::vector<glm::mat3> transformStack_;
    std::vector<glm::vec4> scissorStack_;
    
    int32_t currentZIndex_ = 0;
    glm::vec2 viewportSize_{1920.0f, 1080.0f};
    bool isDirty_ = true;  // True if commands need to be re-rendered
    bool isInOverlayMode_ = false;  // True when collecting overlay commands
    
    // OpenGL resources
    uint32_t vao_ = 0;
    uint32_t vbo_ = 0;
    uint32_t ebo_ = 0;
    uint32_t shaderProgram_ = 0;
    
    bool initialized_ = false;
};

}  // namespace se::ui
