#pragma once

#include <glm.hpp>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

// Forward declaration - Camera is in global namespace
class Camera;

namespace se {

// Forward declarations
class Shader;
class VertexArray;
class VertexBuffer;

namespace ui {
    class UIFont;
}

using Vector3 = glm::vec3;
using Vector4 = glm::vec4;

// Singleton debug renderer for engine-wide 3D debug visualization
// Used by physics, navigation, AI, and any other system needing debug drawing
class DebugRenderer {
   public:
    static DebugRenderer& Get();

    // Immediate mode drawing (cleared each frame after Flush)
    void DrawLine(const Vector3& from, const Vector3& to, const Vector3& color);
    void DrawSphere(const Vector3& center, float radius, const Vector3& color, int segments = 12);
    void DrawBox(const Vector3& min, const Vector3& max, const Vector3& color);
    void DrawPoint(const Vector3& pos, float size, const Vector3& color);
    void DrawArrow(const Vector3& from, const Vector3& to, const Vector3& color, float headSize = 0.2f);
    void DrawCircle(const Vector3& center, float radius, const Vector3& color, 
                    const Vector3& normal = Vector3(0, 1, 0), int segments = 16);
    void DrawPath(const std::vector<Vector3>& waypoints, const Vector3& color, float waypointSize = 0.15f);
    void DrawSquare(const Vector3& center, float halfSize, const Vector3& color,
                    const Vector3& normal = Vector3(0, 1, 0));
    
    // 3D World Text (billboard - always faces camera)
    void DrawText3D(const Vector3& worldPos, const std::string& text, const Vector3& color, float scale = 1.0f);
    void DrawPersistentText3D(const Vector3& worldPos, const std::string& text, const Vector3& color, 
                              float duration, float scale = 1.0f);

    // Persistent drawing (remains until duration expires)
    void DrawPersistentLine(const Vector3& from, const Vector3& to, const Vector3& color, float duration);
    void DrawPersistentSphere(const Vector3& center, float radius, const Vector3& color, 
                              float duration, int segments = 12);
    void DrawPersistentPoint(const Vector3& pos, float size, const Vector3& color, float duration);

    // Frame lifecycle
    void Update(float deltaTime);  // Call each frame to update persistent timers
    void Flush(const Camera& camera);  // Render all and clear immediate

    // Control
    void SetEnabled(bool enabled) { enabled_ = enabled; }
    bool IsEnabled() const { return enabled_; }
    void Clear();  // Clear all immediate drawings

   private:
    DebugRenderer();
    ~DebugRenderer() = default;
    DebugRenderer(const DebugRenderer&) = delete;
    DebugRenderer& operator=(const DebugRenderer&) = delete;

    void InitializeResources();

    struct DebugLine {
        Vector3 from;
        Vector3 to;
        Vector3 color;
    };

    struct PersistentLine {
        Vector3 from;
        Vector3 to;
        Vector3 color;
        float remainingTime;
    };
    
    struct DebugText {
        Vector3 position;
        std::string text;
        Vector3 color;
        float scale;
    };
    
    struct PersistentText {
        Vector3 position;
        std::string text;
        Vector3 color;
        float scale;
        float remainingTime;
    };

    static constexpr uint32_t kInitialLineCapacity = 2048;

    std::vector<DebugLine> lines_;
    std::vector<PersistentLine> persistentLines_;
    std::vector<DebugText> texts_;
    std::vector<PersistentText> persistentTexts_;
    
    std::shared_ptr<Shader> shader_;
    std::shared_ptr<VertexArray> vertexArray_;
    std::shared_ptr<VertexBuffer> vertexBuffer_;
    uint32_t bufferCapacity_ = 0;
    
    // Text rendering resources
    std::shared_ptr<Shader> textShader_;
    std::shared_ptr<ui::UIFont> debugFont_;
    uint32_t textVao_ = 0;
    uint32_t textVbo_ = 0;
    
    void InitializeTextResources();
    void RenderTexts(const Camera& camera);

    bool enabled_ = true;
    bool initialized_ = false;
    std::mutex mutex_;
};

// Convenience namespace for global access
namespace Debug {
    inline void Line(const Vector3& from, const Vector3& to, const Vector3& color) {
        DebugRenderer::Get().DrawLine(from, to, color);
    }
    inline void Sphere(const Vector3& center, float radius, const Vector3& color, int segments = 12) {
        DebugRenderer::Get().DrawSphere(center, radius, color, segments);
    }
    inline void Box(const Vector3& min, const Vector3& max, const Vector3& color) {
        DebugRenderer::Get().DrawBox(min, max, color);
    }
    inline void Point(const Vector3& pos, float size, const Vector3& color) {
        DebugRenderer::Get().DrawPoint(pos, size, color);
    }
    inline void Arrow(const Vector3& from, const Vector3& to, const Vector3& color, float headSize = 0.2f) {
        DebugRenderer::Get().DrawArrow(from, to, color, headSize);
    }
    inline void Path(const std::vector<Vector3>& waypoints, const Vector3& color, float waypointSize = 0.15f) {
        DebugRenderer::Get().DrawPath(waypoints, color, waypointSize);
    }
    inline void Text3D(const Vector3& pos, const std::string& text, const Vector3& color, float scale = 1.0f) {
        DebugRenderer::Get().DrawText3D(pos, text, color, scale);
    }
}  // namespace Debug

// Common debug colors
namespace DebugColors {
    constexpr Vector3 Red     = {1.0f, 0.0f, 0.0f};
    constexpr Vector3 Green   = {0.0f, 1.0f, 0.0f};
    constexpr Vector3 Blue    = {0.0f, 0.0f, 1.0f};
    constexpr Vector3 Yellow  = {1.0f, 1.0f, 0.0f};
    constexpr Vector3 Cyan    = {0.0f, 1.0f, 1.0f};
    constexpr Vector3 Magenta = {1.0f, 0.0f, 1.0f};
    constexpr Vector3 White   = {1.0f, 1.0f, 1.0f};
    constexpr Vector3 Orange  = {1.0f, 0.5f, 0.0f};
    constexpr Vector3 Purple  = {0.5f, 0.0f, 1.0f};
}  // namespace DebugColors

}  // namespace se
