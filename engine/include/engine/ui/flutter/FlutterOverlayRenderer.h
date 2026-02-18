#pragma once

#include <cstdint>

// Forward declarations to avoid including Filament headers.
namespace filament {
class Engine;
class Scene;
class View;
class Camera;
class Texture;
class Material;
class MaterialInstance;
class VertexBuffer;
class IndexBuffer;
}  // namespace filament

namespace utils {
class Entity;
}

namespace se {
class FilamentContext;
class FilamentRenderer;
}  // namespace se

namespace se::ui::flutter {

/**
 * Renders a Flutter-provided RGBA pixel buffer as a Filament overlay.
 *
 * Creates a dedicated Scene/View/Camera stack with TRANSLUCENT blending
 * and composites a fullscreen textured quad on top of the main scene —
 * the same overlay pattern used by ImGuiRenderer and NativeUiRenderer.
 *
 * Layer mask 0x20 (layer 5) positions Flutter below ImGui (layer 6)
 * but above the main scene in the compositing order.
 */
class FlutterOverlayRenderer {
public:
    FlutterOverlayRenderer() = default;
    ~FlutterOverlayRenderer();

    // Disallow copy/move — owns Filament resources.
    FlutterOverlayRenderer(const FlutterOverlayRenderer&) = delete;
    FlutterOverlayRenderer& operator=(const FlutterOverlayRenderer&) = delete;

    /**
     * Initialize the overlay renderer.
     *
     * @param context  Active Filament context (owns Engine).
     * @param renderer Filament renderer for overlay view registration.
     * @param width    Initial framebuffer width in pixels.
     * @param height   Initial framebuffer height in pixels.
     */
    void Init(FilamentContext* context, FilamentRenderer* renderer,
              uint32_t width, uint32_t height);

    /** Tear down all Filament resources. Safe to call multiple times. */
    void Shutdown();

    /** @return true if Init succeeded and Shutdown hasn't been called. */
    bool IsInitialized() const;

    /**
     * Handle a window/framebuffer resize.
     *
     * Recreates the texture and quad geometry at the new resolution.
     */
    void OnResize(uint32_t fbWidth, uint32_t fbHeight,
                  uint32_t winWidth, uint32_t winHeight);

    /**
     * Upload an RGBA pixel buffer from Flutter's software renderer.
     *
     * This is called from the embedder's SoftwareSurfacePresent callback
     * on every frame where the Flutter UI has changed. A dirty flag is
     * set so that EndFrame knows to re-upload.
     *
     * @param pixelData  Pointer to row-major RGBA8888 pixel data.
     * @param rowBytes   Number of bytes per row (may include padding).
     * @param width      Width of the pixel buffer.
     * @param height     Height of the pixel buffer.
     */
    void UpdateTexture(const void* pixelData, size_t rowBytes,
                       uint32_t width, uint32_t height);

    /** Called at the start of each frame (before layer rendering). */
    void BeginFrame();

    /** Called at the end of each frame (commits texture upload). */
    void EndFrame();

private:
    // Resource lifecycle helpers (mirror ImGuiRenderer pattern).
    void CreateResources();
    void DestroyResources();
    void CreateSceneViewCamera();
    void DestroySceneViewCamera();
    void CreateMaterial();
    void DestroyMaterial();
    void CreateFullscreenQuad();
    void DestroyFullscreenQuad();
    void CreateTexture(uint32_t width, uint32_t height);
    void DestroyTexture();
    void UpdateCameraProjection();

    // Layer mask for the overlay view — layer 5.
    static constexpr uint8_t kFlutterLayerMask = 0x20;

    // Dependencies (non-owning).
    FilamentContext*  context_  = nullptr;
    FilamentRenderer* renderer_ = nullptr;

    // Filament overlay scene/view/camera.
    filament::Scene*  scene_  = nullptr;
    filament::View*   view_   = nullptr;
    filament::Camera* camera_ = nullptr;
    bool has_camera_entity_ = false;

    // Material for the textured quad.
    filament::Material*         material_          = nullptr;
    filament::MaterialInstance* material_instance_  = nullptr;

    // Fullscreen quad geometry.
    filament::VertexBuffer* vertex_buffer_ = nullptr;
    filament::IndexBuffer*  index_buffer_  = nullptr;
    bool has_quad_entity_ = false;

    // RGBA texture that receives Flutter pixel data.
    filament::Texture* texture_ = nullptr;

    // Dimensions.
    uint32_t framebuffer_width_  = 0;
    uint32_t framebuffer_height_ = 0;
    uint32_t window_width_       = 0;
    uint32_t window_height_      = 0;

    // Dirty tracking — skip texture upload when Flutter UI hasn't changed.
    bool texture_dirty_ = false;

    bool initialized_ = false;
};

}  // namespace se::ui::flutter
