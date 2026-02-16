#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vector>

#include <utils/Entity.h>

struct ImFont;
struct ImFontAtlas;

namespace filament {
class Camera;
class IndexBuffer;
class Material;
class MaterialInstance;
class Scene;
class Texture;
class VertexBuffer;
class View;
}

namespace se {

class FilamentContext;
class FilamentRenderer;

namespace ui {

struct NativeUiColor {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float a = 1.0f;
};

class NativeUiRenderer {
   public:
    using RetainedSourceId = uint64_t;
    static constexpr RetainedSourceId kInvalidRetainedSourceId = 0;

    enum class TextHorizontalAlign : uint8_t {
        Left = 0,
        Center,
        Right,
    };

    enum class TextVerticalAlign : uint8_t {
        Top = 0,
        Center,
        Bottom,
    };

    struct TextPadding {
        float left   = 0.0f;
        float top    = 0.0f;
        float right  = 0.0f;
        float bottom = 0.0f;
    };

    struct TextLayoutMetrics {
        float advanceWidth = 0.0f;
        float lineHeight   = 0.0f;
        float minX         = 0.0f;
        float maxX         = 0.0f;
        float minY         = 0.0f;
        float maxY         = 0.0f;
        bool  hasVisibleInk = false;

        float InkWidth() const {
            return std::max(0.0f, maxX - minX);
        }
        float InkHeight() const {
            return std::max(0.0f, maxY - minY);
        }
    };

    struct FrameStats {
        uint32_t drawCallsIssued  = 0;
        uint32_t quadsSubmitted   = 0;
        uint32_t verticesUploaded = 0;
        uint32_t indicesUploaded  = 0;
        bool     geometryUploaded = false;
        bool     geometryReused   = false;
    };

    static NativeUiRenderer& Get();

    void Init(FilamentContext* context, FilamentRenderer* renderer, uint32_t width, uint32_t height);
    void Shutdown();

    bool IsInitialized() const {
        return initialized_;
    }

    // framebufferWidth/Height = pixel dimensions (for Filament viewport).
    // windowWidth/Height      = screen-point dimensions (for UI layout & mouse hit-testing).
    // On non-HiDPI displays they are equal; on Retina they differ by the scale factor.
    void OnResize(uint32_t framebufferWidth, uint32_t framebufferHeight,
                  uint32_t windowWidth, uint32_t windowHeight);

    void BeginFrame();
    void EndFrame();
    bool BeginRetainedSource(RetainedSourceId sourceId, bool sourceDirty);
    void EndRetainedSource();
    void RetainPreviousFrameGeometry();
    void InvalidateRetainedGeometry();
    void SetRetainedGeometryReuseEnabled(bool enabled) {
        retained_geometry_reuse_enabled_ = enabled;
    }
    bool IsRetainedGeometryReuseEnabled() const {
        return retained_geometry_reuse_enabled_;
    }
    void SetFlipUvV(bool flip) {
        flip_uv_v_ = flip;
        InvalidateRetainedGeometry();
    }
    bool IsFlipUvV() const {
        return flip_uv_v_;
    }

    void DrawFilledRect(float x, float y, float width, float height, const NativeUiColor& color);
    void DrawRect(float x, float y, float width, float height, float thickness,
                  const NativeUiColor& color);
    void DrawText(std::string_view text, float x, float y, const NativeUiColor& color,
                  float scale = 1.0f);
    TextLayoutMetrics MeasureTextLayout(std::string_view text, float scale = 1.0f) const;
    void DrawTextAligned(std::string_view text, float x, float y, float width, float height,
                         const NativeUiColor& color, float scale,
                         TextHorizontalAlign horizontalAlign,
                         TextVerticalAlign verticalAlign,
                         const TextPadding& padding,
                         bool useInkBounds = true);
    void DrawTextAligned(std::string_view text, float x, float y, float width, float height,
                         const NativeUiColor& color, float scale = 1.0f,
                         TextHorizontalAlign horizontalAlign = TextHorizontalAlign::Left,
                         TextVerticalAlign verticalAlign = TextVerticalAlign::Top,
                         bool useInkBounds = true);

    float GetLineHeight(float scale = 1.0f) const;
    float MeasureTextWidth(std::string_view text, float scale = 1.0f) const;

    float GetViewportWidth() const {
        return static_cast<float>(viewport_width_);
    }

    float GetViewportHeight() const {
        return static_cast<float>(viewport_height_);
    }

    const FrameStats& GetLastFrameStats() const {
        return last_frame_stats_;
    }

   private:
    NativeUiRenderer() = default;
    ~NativeUiRenderer() = default;

    NativeUiRenderer(const NativeUiRenderer&) = delete;
    NativeUiRenderer& operator=(const NativeUiRenderer&) = delete;

    void CreateResources();
    void DestroyResources();
    void RecreateGeometryBuffers(size_t requiredVertexCount, size_t requiredIndexCount);
    void UploadGeometry();
    void UpdateCameraProjection();
    void PrepareForDraw();
    void ComposeFrameGeometryFromSources();
    struct SourceGeometry;
    SourceGeometry* GetWritableSource();

    void PushQuad(float x0, float y0, float x1, float y1, float u0, float v0, float u1, float v1,
                  const NativeUiColor& color);

    static uint8_t ToByte(float value);

    FilamentContext*  context_  = nullptr;
    FilamentRenderer* renderer_ = nullptr;

    // Screen-point dimensions — used for ortho projection, UI layout, mouse coords.
    uint32_t viewport_width_  = 1;
    uint32_t viewport_height_ = 1;
    // Framebuffer pixel dimensions — used only for Filament View::setViewport.
    uint32_t framebuffer_width_  = 1;
    uint32_t framebuffer_height_ = 1;
    bool initialized_         = false;

    ImFontAtlas* font_atlas_ = nullptr;
    ImFont*      font_       = nullptr;
    float        font_base_size_ = 13.0f;

    struct UiVertex {
        float   position[3] = {0.0f, 0.0f, 0.0f};
        float   uv[2]       = {0.0f, 0.0f};
        uint8_t color[4]    = {255, 255, 255, 255};
    };

    struct SourceGeometry {
        std::vector<UiVertex> vertices;
        std::vector<uint32_t> indices;
        bool activeThisFrame = false;
    };

    filament::Scene*            scene_             = nullptr;
    filament::View*             view_              = nullptr;
    filament::Camera*           camera_            = nullptr;
    filament::Material*         material_          = nullptr;
    filament::MaterialInstance* material_instance_ = nullptr;
    filament::Texture*          atlas_texture_     = nullptr;
    filament::VertexBuffer*     vertex_buffer_     = nullptr;
    filament::IndexBuffer*      index_buffer_      = nullptr;
    utils::Entity               camera_entity_;
    utils::Entity               renderable_entity_;
    bool                        has_camera_entity_     = false;
    bool                        has_renderable_entity_ = false;

    std::vector<UiVertex>  vertices_;
    std::vector<uint32_t>  indices_;
    size_t                 vertex_capacity_ = 0;
    size_t                 index_capacity_  = 0;
    float                  white_uv_x_      = 0.0f;
    float                  white_uv_y_      = 0.0f;

    std::unordered_map<RetainedSourceId, SourceGeometry> retained_sources_;
    std::vector<RetainedSourceId> frame_source_order_;
    std::vector<RetainedSourceId> previous_source_order_;

    RetainedSourceId active_source_id_ = kInvalidRetainedSourceId;
    bool             active_source_recording_ = false;
    bool             frame_sources_dirty_ = false;
    bool             frame_topology_dirty_ = false;

    bool retain_previous_geometry_ = false;
    bool frame_reuse_active_       = false;
    bool geometry_dirty_           = true;
    bool has_geometry_             = false;
    bool retained_geometry_reuse_enabled_ = true;
    bool flip_uv_v_ = false;

    FrameStats last_frame_stats_;
    uint32_t   frame_quads_submitted_ = 0;
};

}  // namespace ui

}  // namespace se
