#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "Engine.h"

#include <utils/Entity.h>

struct ImGuiContext;

namespace filament {
class Camera;
class IndexBuffer;
class Material;
class MaterialInstance;
class Scene;
class Texture;
class VertexBuffer;
class View;
class TextureSampler;
}  // namespace filament

namespace se {

class EventBus;
class FilamentContext;
class FilamentRenderer;
class Window;

namespace ui::imgui {

class ImGuiRenderer {
   public:
    ImGuiRenderer() = default;
    ~ImGuiRenderer();

    ImGuiRenderer(const ImGuiRenderer&) = delete;
    ImGuiRenderer& operator=(const ImGuiRenderer&) = delete;

    void Init(FilamentContext* context, FilamentRenderer* renderer, Window* window,
              EventBus* eventBus, const std::string& appName);
    void Shutdown();

    void OnResize(uint32_t framebufferWidth, uint32_t framebufferHeight, uint32_t windowWidth,
                  uint32_t windowHeight);

    void BeginFrame(float deltaTimeSeconds);
    void EndFrame();

    bool IsInitialized() const {
        return initialized_;
    }
    bool IsFrameActive() const {
        return frame_active_;
    }

    bool WantsCaptureMouse() const {
        return wants_capture_mouse_;
    }
    bool WantsCaptureKeyboard() const {
        return wants_capture_keyboard_;
    }
    bool WantsCaptureInput() const {
        return wants_capture_mouse_ || wants_capture_keyboard_;
    }

   private:
    struct Vertex {
        float   position[3] = {0.0f, 0.0f, 0.0f};
        float   uv[2]       = {0.0f, 0.0f};
        uint8_t color[4]    = {255, 255, 255, 255};
    };

    struct DrawPrimitive {
        uint32_t index_offset = 0;
        uint32_t index_count  = 0;
        filament::Texture* texture = nullptr;
        float clip_min_x = 0.0f;
        float clip_min_y = 0.0f;
        float clip_max_x = 0.0f;
        float clip_max_y = 0.0f;
    };

    struct KeyBinding {
        KeyCode keycode = 0;
        int keyIndex = 0;
        bool previous_down = false;
    };

    void CreateResources();
    void DestroyResources();
    void CreateImGuiContext();
    void DestroyImGuiContext();
    void CreateFontTexture();
    void DestroyFontTexture();
    void CreateMaterial();
    void DestroyMaterial();
    void CreateSceneViewCamera();
    void DestroySceneViewCamera();

    void EnsureGeometryCapacity(size_t requiredVertexCount, size_t requiredIndexCount);
    void EnsurePrimitiveCapacity(size_t requiredPrimitiveCount);
    void RecreateGeometryBuffers();
    void RecreateRenderable();
    void DestroyRenderable();
    void UploadGeometry();
    void BuildDrawData();
    void UpdateCameraProjection();
    void UpdateInputState();
    void FlushQueuedCharacters();

    filament::Texture* ResolveTexture(void* textureId) const;

    void OnTextInput(uint32_t codepoint);

    FilamentContext*  context_   = nullptr;
    FilamentRenderer* renderer_  = nullptr;
    Window*           window_    = nullptr;
    EventBus*         event_bus_ = nullptr;

    bool initialized_ = false;
    bool frame_active_ = false;

    uint32_t framebuffer_width_  = 1;
    uint32_t framebuffer_height_ = 1;
    uint32_t window_width_       = 1;
    uint32_t window_height_      = 1;

    bool wants_capture_mouse_    = false;
    bool wants_capture_keyboard_ = false;

    filament::Scene*  scene_  = nullptr;
    filament::View*   view_   = nullptr;
    filament::Camera* camera_ = nullptr;

    utils::Entity camera_entity_;
    bool          has_camera_entity_ = false;

    filament::Material* material_ = nullptr;
    std::vector<filament::MaterialInstance*> material_instances_;
    size_t primitive_capacity_ = 0;

    filament::Texture*        font_texture_ = nullptr;
    filament::TextureSampler* texture_sampler_ = nullptr;

    filament::VertexBuffer* vertex_buffer_ = nullptr;
    filament::IndexBuffer*  index_buffer_  = nullptr;
    size_t                  vertex_capacity_ = 0;
    size_t                  index_capacity_  = 0;

    utils::Entity renderable_entity_;
    bool          has_renderable_entity_ = false;

    std::vector<Vertex> vertices_;
    std::vector<uint32_t> indices_;
    std::vector<DrawPrimitive> draw_primitives_;

    std::vector<KeyBinding> key_bindings_;
    bool mouse_button_state_[5] = {false, false, false, false, false};
    bool modifier_state_ctrl_  = false;
    bool modifier_state_shift_ = false;
    bool modifier_state_alt_   = false;
    bool modifier_state_super_ = false;
    float last_mouse_x_ = 0.0f;
    float last_mouse_y_ = 0.0f;
    bool has_last_mouse_position_ = false;

    std::vector<uint32_t> queued_characters_;

    std::optional<size_t> text_input_listener_id_;

    std::string ini_file_path_;
    ImGuiContext* imgui_context_ = nullptr;
};

}  // namespace ui::imgui

}  // namespace se
