#include "engine/ui/imgui/ImGuiRenderer.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cfloat>
#include <cstring>
#include <limits>
#include <string>

#include <GLFW/glfw3.h>

#include <filament/Camera.h>
#include <filament/Engine.h>
#include <filament/IndexBuffer.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>
#include <filament/RenderableManager.h>
#include <filament/Scene.h>
#include <filament/Texture.h>
#include <filament/TextureSampler.h>
#include <filament/VertexBuffer.h>
#include <filament/View.h>
#include <filament/Viewport.h>

#include <filamat/MaterialBuilder.h>

#include <imgui.h>

#include <math/vec4.h>

#include <utils/EntityManager.h>

#include "engine/Log.h"
#include "engine/Window.h"
#include "engine/events/EventBus.h"
#include "engine/events/Events.h"
#include "engine/input/InputManager.h"
#include "engine/input/KeyCodes.h"
#include "engine/renderer/FilamentContext.h"
#include "engine/renderer/FilamentRenderer.h"

namespace se::ui::imgui {

namespace {

constexpr uint8_t kImGuiLayerMask = 0x40;  // layer 6
constexpr const char* kImGuiMaterialSource = R"FILAMENT(
void material(inout MaterialInputs material) {
    prepareMaterial(material);
    float4 color = getColor();
    float4 texel = texture(materialParams_uiTexture, getUV0());
    // Filament TRANSPARENT expects premultiplied alpha.
    float alpha = texel.a * color.a;
    material.baseColor = float4(texel.rgb * color.rgb * alpha, alpha);
}
)FILAMENT";

size_t NextPow2(size_t value) {
    size_t v = std::max<size_t>(1, value);
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    if constexpr (sizeof(size_t) == 8) {
        v |= v >> 32;
    }
    v++;
    return v;
}

void CopyImGuiColorToBytes(ImU32 packedColor, uint8_t outColor[4]) {
    std::memcpy(outColor, &packedColor, sizeof(ImU32));
}

}  // namespace

ImGuiRenderer::~ImGuiRenderer() {
    Shutdown();
}

void ImGuiRenderer::Init(FilamentContext* context, FilamentRenderer* renderer, Window* window,
                         EventBus* eventBus, const std::string& appName) {
    if (initialized_) return;
    if (!context || !renderer || !window || !eventBus) {
        SE_LOG_ERROR("ImGuiRenderer::Init failed: null dependency");
        return;
    }

    context_   = context;
    renderer_  = renderer;
    window_    = window;
    event_bus_ = eventBus;

    int fbWidth = 1;
    int fbHeight = 1;
    int winWidth = 1;
    int winHeight = 1;
    glfwGetFramebufferSize(window_->GetNativeWindow(), &fbWidth, &fbHeight);
    glfwGetWindowSize(window_->GetNativeWindow(), &winWidth, &winHeight);
    OnResize(static_cast<uint32_t>(std::max(1, fbWidth)),
             static_cast<uint32_t>(std::max(1, fbHeight)),
             static_cast<uint32_t>(std::max(1, winWidth)),
             static_cast<uint32_t>(std::max(1, winHeight)));

    ini_file_path_ = "assets/imgui_" + appName + ".ini";

    CreateResources();
    if (!initialized_) {
        Shutdown();
        return;
    }

    text_input_listener_id_ = event_bus_->AddListener<TextInputEvent>(
        [this](const TextInputEvent& event) { OnTextInput(event.codepoint); });

    SE_LOG_INFO("ImGuiRenderer initialized (Filament overlay)");
}

void ImGuiRenderer::Shutdown() {
    if (!context_ && !renderer_ && !event_bus_ && !window_ && !initialized_) {
        return;
    }

    if (event_bus_ && text_input_listener_id_.has_value()) {
        event_bus_->RemoveListener<TextInputEvent>(*text_input_listener_id_);
        text_input_listener_id_.reset();
    }

    DestroyResources();

    context_   = nullptr;
    renderer_  = nullptr;
    window_    = nullptr;
    event_bus_ = nullptr;

    initialized_ = false;
    frame_active_ = false;
    wants_capture_mouse_ = false;
    wants_capture_keyboard_ = false;
}

void ImGuiRenderer::OnResize(uint32_t framebufferWidth, uint32_t framebufferHeight,
                             uint32_t windowWidth, uint32_t windowHeight) {
    framebuffer_width_  = std::max<uint32_t>(1, framebufferWidth);
    framebuffer_height_ = std::max<uint32_t>(1, framebufferHeight);
    window_width_       = std::max<uint32_t>(1, windowWidth);
    window_height_      = std::max<uint32_t>(1, windowHeight);

    if (!initialized_) return;

    if (view_) {
        view_->setViewport({0, 0, framebuffer_width_, framebuffer_height_});
    }
    UpdateCameraProjection();

    auto* engine = context_ ? context_->GetEngine() : nullptr;
    if (!engine || !has_renderable_entity_) return;
    auto& renderableManager = engine->getRenderableManager();
    const auto instance = renderableManager.getInstance(renderable_entity_);
    if (!instance.isValid()) return;
    renderableManager.setAxisAlignedBoundingBox(
        instance,
        {{window_width_ * 0.5f, window_height_ * 0.5f, 0.0f},
         {window_width_ * 0.5f, window_height_ * 0.5f, 1.0f}});
}

void ImGuiRenderer::BeginFrame(float deltaTimeSeconds) {
    if (!initialized_) {
        static bool s_reported_uninitialized = false;
        if (!s_reported_uninitialized) {
            SE_LOG_ERROR("ImGuiRenderer::BeginFrame skipped because renderer is not initialized.");
            s_reported_uninitialized = true;
        }
        return;
    }
    if (frame_active_) return;

    ImGui::SetCurrentContext(imgui_context_);
    ImGuiIO& io = ImGui::GetIO();

    io.DeltaTime = std::max(0.0001f, deltaTimeSeconds);
    io.DisplaySize = ImVec2(static_cast<float>(window_width_), static_cast<float>(window_height_));
    io.DisplayFramebufferScale = ImVec2(
        static_cast<float>(framebuffer_width_) / static_cast<float>(window_width_),
        static_cast<float>(framebuffer_height_) / static_cast<float>(window_height_));

    UpdateInputState();
    FlushQueuedCharacters();

    ImGui::NewFrame();
    frame_active_ = true;
}

void ImGuiRenderer::EndFrame() {
    if (!initialized_ || !frame_active_) return;

    ImGui::SetCurrentContext(imgui_context_);

    ImGui::Render();
    BuildDrawData();
    UploadGeometry();

    const ImGuiIO& io = ImGui::GetIO();
    wants_capture_mouse_ = io.WantCaptureMouse;
    wants_capture_keyboard_ = io.WantCaptureKeyboard;

    frame_active_ = false;
}

void ImGuiRenderer::CreateResources() {
    CreateImGuiContext();
    if (!imgui_context_) {
        SE_LOG_ERROR("ImGuiRenderer resource creation aborted: context creation failed.");
        return;
    }

    CreateFontTexture();
    if (!font_texture_) {
        SE_LOG_ERROR("ImGuiRenderer resource creation aborted: font texture creation failed.");
        return;
    }

    CreateMaterial();
    if (!material_) {
        SE_LOG_ERROR("ImGuiRenderer resource creation aborted: material creation failed.");
        return;
    }

    CreateSceneViewCamera();
    if (!scene_ || !view_ || !camera_) {
        SE_LOG_ERROR("ImGuiRenderer resource creation aborted: scene/view/camera creation failed.");
        return;
    }

    vertex_capacity_ = 4096;
    index_capacity_  = 8192;
    RecreateGeometryBuffers();
    if (!vertex_buffer_ || !index_buffer_) {
        SE_LOG_ERROR("ImGuiRenderer resource creation aborted: geometry buffer creation failed.");
        return;
    }

    primitive_capacity_ = 32;
    RecreateRenderable();
    if (!has_renderable_entity_) {
        SE_LOG_ERROR("ImGuiRenderer resource creation aborted: renderable creation failed.");
        return;
    }

    initialized_ = true;
}

void ImGuiRenderer::DestroyResources() {
    DestroyRenderable();

    auto* engine = context_ ? context_->GetEngine() : nullptr;
    if (engine && vertex_buffer_) {
        engine->destroy(vertex_buffer_);
    }
    if (engine && index_buffer_) {
        engine->destroy(index_buffer_);
    }
    vertex_buffer_ = nullptr;
    index_buffer_  = nullptr;
    vertex_capacity_ = 0;
    index_capacity_  = 0;

    DestroySceneViewCamera();
    DestroyMaterial();
    DestroyFontTexture();
    DestroyImGuiContext();

    vertices_.clear();
    indices_.clear();
    draw_primitives_.clear();
    key_bindings_.clear();
    queued_characters_.clear();
    has_last_mouse_position_ = false;
    modifier_state_ctrl_ = false;
    modifier_state_shift_ = false;
    modifier_state_alt_ = false;
    modifier_state_super_ = false;
}

void ImGuiRenderer::CreateImGuiContext() {
    IMGUI_CHECKVERSION();
    imgui_context_ = ImGui::CreateContext();
    if (!imgui_context_) {
        SE_LOG_ERROR("ImGuiRenderer failed to create Dear ImGui context");
        return;
    }

    ImGui::SetCurrentContext(imgui_context_);
    ImGuiIO& io = ImGui::GetIO();

    io.IniFilename = ini_file_path_.c_str();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;

    io.BackendRendererName = "MonsterEngine_ImGuiFilament";
    io.BackendPlatformName = "MonsterEngine_CustomInput";
    io.BackendFlags |= ImGuiBackendFlags_RendererHasVtxOffset;

    ImGui::StyleColorsDark();

    key_bindings_ = {
        {Key::Tab, ImGuiKey_Tab, false},
        {Key::Left, ImGuiKey_LeftArrow, false},
        {Key::Right, ImGuiKey_RightArrow, false},
        {Key::Up, ImGuiKey_UpArrow, false},
        {Key::Down, ImGuiKey_DownArrow, false},
        {Key::PageUp, ImGuiKey_PageUp, false},
        {Key::PageDown, ImGuiKey_PageDown, false},
        {Key::Home, ImGuiKey_Home, false},
        {Key::End, ImGuiKey_End, false},
        {Key::Insert, ImGuiKey_Insert, false},
        {Key::Delete, ImGuiKey_Delete, false},
        {Key::Backspace, ImGuiKey_Backspace, false},
        {Key::Space, ImGuiKey_Space, false},
        {Key::Enter, ImGuiKey_Enter, false},
        {Key::Escape, ImGuiKey_Escape, false},
        {Key::Apostrophe, ImGuiKey_Apostrophe, false},
        {Key::Comma, ImGuiKey_Comma, false},
        {Key::Minus, ImGuiKey_Minus, false},
        {Key::Period, ImGuiKey_Period, false},
        {Key::Slash, ImGuiKey_Slash, false},
        {Key::Semicolon, ImGuiKey_Semicolon, false},
        {Key::Equal, ImGuiKey_Equal, false},
        {Key::LeftBracket, ImGuiKey_LeftBracket, false},
        {Key::Backslash, ImGuiKey_Backslash, false},
        {Key::RightBracket, ImGuiKey_RightBracket, false},
        {Key::GraveAccent, ImGuiKey_GraveAccent, false},
        {Key::CapsLock, ImGuiKey_CapsLock, false},
        {Key::ScrollLock, ImGuiKey_ScrollLock, false},
        {Key::NumLock, ImGuiKey_NumLock, false},
        {Key::PrintScreen, ImGuiKey_PrintScreen, false},
        {Key::Pause, ImGuiKey_Pause, false},
        {Key::KP0, ImGuiKey_Keypad0, false},
        {Key::KP1, ImGuiKey_Keypad1, false},
        {Key::KP2, ImGuiKey_Keypad2, false},
        {Key::KP3, ImGuiKey_Keypad3, false},
        {Key::KP4, ImGuiKey_Keypad4, false},
        {Key::KP5, ImGuiKey_Keypad5, false},
        {Key::KP6, ImGuiKey_Keypad6, false},
        {Key::KP7, ImGuiKey_Keypad7, false},
        {Key::KP8, ImGuiKey_Keypad8, false},
        {Key::KP9, ImGuiKey_Keypad9, false},
        {Key::KPDecimal, ImGuiKey_KeypadDecimal, false},
        {Key::KPDivide, ImGuiKey_KeypadDivide, false},
        {Key::KPMultiply, ImGuiKey_KeypadMultiply, false},
        {Key::KPSubtract, ImGuiKey_KeypadSubtract, false},
        {Key::KPAdd, ImGuiKey_KeypadAdd, false},
        {Key::KPEnter, ImGuiKey_KeypadEnter, false},
        {Key::KPEqual, ImGuiKey_KeypadEqual, false},
        {Key::LeftShift, ImGuiKey_LeftShift, false},
        {Key::LeftControl, ImGuiKey_LeftCtrl, false},
        {Key::LeftAlt, ImGuiKey_LeftAlt, false},
        {Key::LeftSuper, ImGuiKey_LeftSuper, false},
        {Key::RightShift, ImGuiKey_RightShift, false},
        {Key::RightControl, ImGuiKey_RightCtrl, false},
        {Key::RightAlt, ImGuiKey_RightAlt, false},
        {Key::RightSuper, ImGuiKey_RightSuper, false},
        {Key::Menu, ImGuiKey_Menu, false},
        {Key::D0, ImGuiKey_0, false},
        {Key::D1, ImGuiKey_1, false},
        {Key::D2, ImGuiKey_2, false},
        {Key::D3, ImGuiKey_3, false},
        {Key::D4, ImGuiKey_4, false},
        {Key::D5, ImGuiKey_5, false},
        {Key::D6, ImGuiKey_6, false},
        {Key::D7, ImGuiKey_7, false},
        {Key::D8, ImGuiKey_8, false},
        {Key::D9, ImGuiKey_9, false},
        {Key::A, ImGuiKey_A, false},
        {Key::B, ImGuiKey_B, false},
        {Key::C, ImGuiKey_C, false},
        {Key::D, ImGuiKey_D, false},
        {Key::E, ImGuiKey_E, false},
        {Key::F, ImGuiKey_F, false},
        {Key::G, ImGuiKey_G, false},
        {Key::H, ImGuiKey_H, false},
        {Key::I, ImGuiKey_I, false},
        {Key::J, ImGuiKey_J, false},
        {Key::K, ImGuiKey_K, false},
        {Key::L, ImGuiKey_L, false},
        {Key::M, ImGuiKey_M, false},
        {Key::N, ImGuiKey_N, false},
        {Key::O, ImGuiKey_O, false},
        {Key::P, ImGuiKey_P, false},
        {Key::Q, ImGuiKey_Q, false},
        {Key::R, ImGuiKey_R, false},
        {Key::S, ImGuiKey_S, false},
        {Key::T, ImGuiKey_T, false},
        {Key::U, ImGuiKey_U, false},
        {Key::V, ImGuiKey_V, false},
        {Key::W, ImGuiKey_W, false},
        {Key::X, ImGuiKey_X, false},
        {Key::Y, ImGuiKey_Y, false},
        {Key::Z, ImGuiKey_Z, false},
        {Key::F1, ImGuiKey_F1, false},
        {Key::F2, ImGuiKey_F2, false},
        {Key::F3, ImGuiKey_F3, false},
        {Key::F4, ImGuiKey_F4, false},
        {Key::F5, ImGuiKey_F5, false},
        {Key::F6, ImGuiKey_F6, false},
        {Key::F7, ImGuiKey_F7, false},
        {Key::F8, ImGuiKey_F8, false},
        {Key::F9, ImGuiKey_F9, false},
        {Key::F10, ImGuiKey_F10, false},
        {Key::F11, ImGuiKey_F11, false},
        {Key::F12, ImGuiKey_F12, false},
    };
}

void ImGuiRenderer::DestroyImGuiContext() {
    if (!imgui_context_) return;
    ImGui::SetCurrentContext(imgui_context_);
    ImGui::DestroyContext(imgui_context_);
    imgui_context_ = nullptr;
}

void ImGuiRenderer::CreateFontTexture() {
    if (!imgui_context_) return;

    auto* engine = context_ ? context_->GetEngine() : nullptr;
    if (!engine) return;

    ImGui::SetCurrentContext(imgui_context_);
    ImGuiIO& io = ImGui::GetIO();

    unsigned char* pixels = nullptr;
    int width = 0;
    int height = 0;
    io.Fonts->GetTexDataAsRGBA32(&pixels, &width, &height);

    if (!pixels || width <= 0 || height <= 0) {
        SE_LOG_ERROR("ImGuiRenderer failed to build font atlas");
        return;
    }

    font_texture_ = filament::Texture::Builder()
                        .width(static_cast<uint32_t>(width))
                        .height(static_cast<uint32_t>(height))
                        .levels(1)
                        .sampler(filament::Texture::Sampler::SAMPLER_2D)
                        .format(filament::Texture::InternalFormat::RGBA8)
                        .build(*engine);

    if (!font_texture_) {
        SE_LOG_ERROR("ImGuiRenderer failed to create font texture");
        return;
    }

    const size_t byteCount = static_cast<size_t>(width) * static_cast<size_t>(height) * 4u;
    auto* textureCopy = new uint8_t[byteCount];
    std::memcpy(textureCopy, pixels, byteCount);

    font_texture_->setImage(
        *engine, 0,
        filament::Texture::PixelBufferDescriptor(
            textureCopy, byteCount, filament::Texture::Format::RGBA,
            filament::Texture::Type::UBYTE,
            [](void* data, size_t, void*) { delete[] static_cast<uint8_t*>(data); }));

    texture_sampler_ = new filament::TextureSampler(
        filament::TextureSampler::MinFilter::LINEAR,
        filament::TextureSampler::MagFilter::LINEAR,
        filament::TextureSampler::WrapMode::CLAMP_TO_EDGE);

    io.Fonts->SetTexID(static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(font_texture_)));
}

void ImGuiRenderer::DestroyFontTexture() {
    auto* engine = context_ ? context_->GetEngine() : nullptr;
    if (imgui_context_) {
        ImGui::SetCurrentContext(imgui_context_);
        ImGuiIO& io = ImGui::GetIO();
        if (io.Fonts &&
            io.Fonts->TexRef.GetTexID() ==
                static_cast<ImTextureID>(reinterpret_cast<uintptr_t>(font_texture_))) {
            io.Fonts->SetTexID(ImTextureID_Invalid);
        }
    }

    if (engine && font_texture_) {
        engine->destroy(font_texture_);
    }
    font_texture_ = nullptr;

    delete texture_sampler_;
    texture_sampler_ = nullptr;
}

void ImGuiRenderer::CreateMaterial() {
    auto* engine = context_ ? context_->GetEngine() : nullptr;
    if (!engine) return;

    filamat::MaterialBuilder builder;
    builder.name("ImGuiOverlayMaterial")
        .material(kImGuiMaterialSource)
        .shading(filament::Shading::UNLIT)
        .require(filament::VertexAttribute::UV0)
        .require(filament::VertexAttribute::COLOR)
        .parameter("uiTexture", filamat::MaterialBuilder::SamplerType::SAMPLER_2D,
                   filamat::MaterialBuilder::SamplerFormat::FLOAT)
        .blending(filamat::MaterialBuilder::BlendingMode::TRANSPARENT)
        .depthWrite(false)
        .depthCulling(false)
        .doubleSided(true)
        .targetApi(filamat::MaterialBuilder::TargetApi::ALL)
        .platform(filamat::MaterialBuilder::Platform::ALL);

    filamat::Package package = builder.build(engine->getJobSystem());
    if (!package.isValid()) {
        SE_LOG_ERROR("ImGuiRenderer failed to compile material package");
        return;
    }

    material_ =
        filament::Material::Builder().package(package.getData(), package.getSize()).build(*engine);
    if (!material_) {
        SE_LOG_ERROR("ImGuiRenderer failed to create material");
    }
}

void ImGuiRenderer::DestroyMaterial() {
    auto* engine = context_ ? context_->GetEngine() : nullptr;
    if (!engine) {
        material_ = nullptr;
        return;
    }

    for (auto* materialInstance : material_instances_) {
        if (materialInstance) {
            engine->destroy(materialInstance);
        }
    }
    material_instances_.clear();

    if (material_) {
        engine->destroy(material_);
    }
    material_ = nullptr;
}

void ImGuiRenderer::CreateSceneViewCamera() {
    auto* engine = context_ ? context_->GetEngine() : nullptr;
    if (!engine || !renderer_) return;

    scene_ = engine->createScene();
    view_  = engine->createView();
    if (!scene_ || !view_) {
        SE_LOG_ERROR("ImGuiRenderer failed to create Filament scene/view");
        return;
    }

    camera_entity_ = utils::EntityManager::get().create();
    has_camera_entity_ = true;
    camera_ = engine->createCamera(camera_entity_);
    if (!camera_) {
        SE_LOG_ERROR("ImGuiRenderer failed to create camera");
        return;
    }

    view_->setScene(scene_);
    view_->setCamera(camera_);
    view_->setViewport({0, 0, framebuffer_width_, framebuffer_height_});
    view_->setBlendMode(filament::View::BlendMode::TRANSLUCENT);
    view_->setVisibleLayers(0xFF, kImGuiLayerMask);
    view_->setPostProcessingEnabled(false);
    view_->setFrustumCullingEnabled(false);
    view_->setAntiAliasing(filament::AntiAliasing::NONE);
    view_->setDithering(filament::Dithering::NONE);

    UpdateCameraProjection();
    renderer_->RegisterOverlayView(view_);
}

void ImGuiRenderer::DestroySceneViewCamera() {
    auto* engine = context_ ? context_->GetEngine() : nullptr;

    if (renderer_ && view_) {
        renderer_->UnregisterOverlayView(view_);
    }

    if (engine && view_) {
        engine->destroy(view_);
    }
    if (engine && scene_) {
        engine->destroy(scene_);
    }
    view_  = nullptr;
    scene_ = nullptr;

    if (has_camera_entity_) {
        if (engine && camera_) {
            engine->destroyCameraComponent(camera_entity_);
        }
        utils::EntityManager::get().destroy(camera_entity_);
    }
    camera_ = nullptr;
    has_camera_entity_ = false;
}

void ImGuiRenderer::EnsureGeometryCapacity(size_t requiredVertexCount, size_t requiredIndexCount) {
    if (requiredVertexCount <= vertex_capacity_ && requiredIndexCount <= index_capacity_) {
        return;
    }

    vertex_capacity_ = NextPow2(std::max<size_t>(requiredVertexCount, 4096));
    index_capacity_  = NextPow2(std::max<size_t>(requiredIndexCount, 8192));
    RecreateGeometryBuffers();
    RecreateRenderable();
}

void ImGuiRenderer::EnsurePrimitiveCapacity(size_t requiredPrimitiveCount) {
    if (requiredPrimitiveCount <= primitive_capacity_) return;
    primitive_capacity_ = NextPow2(std::max<size_t>(requiredPrimitiveCount, 32));
    RecreateRenderable();
}

void ImGuiRenderer::RecreateGeometryBuffers() {
    auto* engine = context_ ? context_->GetEngine() : nullptr;
    if (!engine) return;

    if (vertex_buffer_) {
        engine->destroy(vertex_buffer_);
        vertex_buffer_ = nullptr;
    }
    if (index_buffer_) {
        engine->destroy(index_buffer_);
        index_buffer_ = nullptr;
    }

    vertex_buffer_ = filament::VertexBuffer::Builder()
                         .vertexCount(static_cast<uint32_t>(vertex_capacity_))
                         .bufferCount(1)
                         .attribute(filament::VertexAttribute::POSITION, 0,
                                    filament::VertexBuffer::AttributeType::FLOAT3,
                                    offsetof(Vertex, position), sizeof(Vertex))
                         .attribute(filament::VertexAttribute::UV0, 0,
                                    filament::VertexBuffer::AttributeType::FLOAT2,
                                    offsetof(Vertex, uv), sizeof(Vertex))
                         .attribute(filament::VertexAttribute::COLOR, 0,
                                    filament::VertexBuffer::AttributeType::UBYTE4,
                                    offsetof(Vertex, color), sizeof(Vertex))
                         .normalized(filament::VertexAttribute::COLOR)
                         .build(*engine);

    index_buffer_ = filament::IndexBuffer::Builder()
                        .indexCount(static_cast<uint32_t>(index_capacity_))
                        .bufferType(filament::IndexBuffer::IndexType::UINT)
                        .build(*engine);

    if (!vertex_buffer_ || !index_buffer_) {
        SE_LOG_ERROR("ImGuiRenderer failed to create geometry buffers");
    }
}

void ImGuiRenderer::RecreateRenderable() {
    auto* engine = context_ ? context_->GetEngine() : nullptr;
    if (!engine || !scene_ || !material_ || !vertex_buffer_ || !index_buffer_ ||
        primitive_capacity_ == 0) {
        return;
    }

    DestroyRenderable();

    material_instances_.reserve(primitive_capacity_);
    for (size_t i = 0; i < primitive_capacity_; ++i) {
        filament::MaterialInstance* instance = material_->createInstance();
        if (instance && font_texture_ && texture_sampler_) {
            instance->setParameter("uiTexture", font_texture_, *texture_sampler_);
        }
        material_instances_.push_back(instance);
    }

    renderable_entity_ = utils::EntityManager::get().create();
    has_renderable_entity_ = true;

    filament::RenderableManager::Builder builder(primitive_capacity_);
    for (size_t i = 0; i < primitive_capacity_; ++i) {
        builder.geometry(i, filament::RenderableManager::PrimitiveType::TRIANGLES, vertex_buffer_,
                         index_buffer_, 0, 0);
        builder.material(i, material_instances_[i]);
        builder.blendOrder(
            i, static_cast<uint16_t>(std::min<size_t>(i, std::numeric_limits<uint16_t>::max())));
        builder.globalBlendOrderEnabled(i, true);
    }

    builder.boundingBox({{window_width_ * 0.5f, window_height_ * 0.5f, 0.0f},
                         {window_width_ * 0.5f, window_height_ * 0.5f, 1.0f}})
        .culling(false)
        .castShadows(false)
        .receiveShadows(false)
        .priority(7)
        .layerMask(0xFF, 0x00)
        .build(*engine, renderable_entity_);

    auto& renderableManager = engine->getRenderableManager();
    const auto instance = renderableManager.getInstance(renderable_entity_);
    if (!instance.isValid()) {
        SE_LOG_ERROR("ImGuiRenderer failed to build renderable entity.");
        utils::EntityManager::get().destroy(renderable_entity_);
        has_renderable_entity_ = false;
        return;
    }

    scene_->addEntity(renderable_entity_);
}

void ImGuiRenderer::DestroyRenderable() {
    auto* engine = context_ ? context_->GetEngine() : nullptr;
    if (engine && has_renderable_entity_) {
        if (scene_) {
            scene_->remove(renderable_entity_);
        }
        engine->destroy(renderable_entity_);
        utils::EntityManager::get().destroy(renderable_entity_);
    }

    has_renderable_entity_ = false;

    if (engine) {
        for (auto* materialInstance : material_instances_) {
            if (materialInstance) {
                engine->destroy(materialInstance);
            }
        }
    }
    material_instances_.clear();
}

void ImGuiRenderer::UploadGeometry() {
    auto* engine = context_ ? context_->GetEngine() : nullptr;
    if (!engine || !vertex_buffer_ || !index_buffer_ || !has_renderable_entity_) return;

    EnsureGeometryCapacity(vertices_.size(), indices_.size());
    EnsurePrimitiveCapacity(draw_primitives_.size());

    if (!has_renderable_entity_) return;

    if (!vertices_.empty()) {
        auto* vertexCopy = new Vertex[vertices_.size()];
        std::memcpy(vertexCopy, vertices_.data(), vertices_.size() * sizeof(Vertex));
        vertex_buffer_->setBufferAt(
            *engine, 0,
            filament::VertexBuffer::BufferDescriptor(
                vertexCopy, vertices_.size() * sizeof(Vertex),
                [](void* data, size_t, void*) { delete[] static_cast<Vertex*>(data); }));
    }

    if (!indices_.empty()) {
        auto* indexCopy = new uint32_t[indices_.size()];
        std::memcpy(indexCopy, indices_.data(), indices_.size() * sizeof(uint32_t));
        index_buffer_->setBuffer(
            *engine, filament::IndexBuffer::BufferDescriptor(
                         indexCopy, indices_.size() * sizeof(uint32_t),
                         [](void* data, size_t, void*) { delete[] static_cast<uint32_t*>(data); }));
    }

    auto& renderableManager = engine->getRenderableManager();
    const auto instance = renderableManager.getInstance(renderable_entity_);
    if (!instance.isValid()) return;

    if (draw_primitives_.empty()) {
        renderableManager.setLayerMask(instance, 0xFF, 0x00);
        for (size_t i = 0; i < primitive_capacity_; ++i) {
            renderableManager.setGeometryAt(instance, i,
                                            filament::RenderableManager::PrimitiveType::TRIANGLES,
                                            vertex_buffer_, index_buffer_, 0, 0);
        }
        return;
    }

    renderableManager.setLayerMask(instance, 0xFF, kImGuiLayerMask);

    for (size_t i = 0; i < draw_primitives_.size(); ++i) {
        const DrawPrimitive& primitive = draw_primitives_[i];
        filament::MaterialInstance* materialInstance = material_instances_[i];
        if (!materialInstance) continue;

        // Update texture on the material instance if it differs from the
        // default font texture (e.g. offscreen viewport textures via ImGui::Image).
        if (primitive.texture && texture_sampler_) {
            materialInstance->setParameter("uiTexture", primitive.texture, *texture_sampler_);
        }

        renderableManager.setMaterialInstanceAt(instance, i, materialInstance);
        renderableManager.setGeometryAt(instance, i,
                                        filament::RenderableManager::PrimitiveType::TRIANGLES,
                                        vertex_buffer_, index_buffer_, primitive.index_offset,
                                        primitive.index_count);
        renderableManager.setBlendOrderAt(
            instance, i,
            static_cast<uint16_t>(std::min<size_t>(i, std::numeric_limits<uint16_t>::max())));
        renderableManager.setGlobalBlendOrderEnabledAt(instance, i, true);
    }

    for (size_t i = draw_primitives_.size(); i < primitive_capacity_; ++i) {
        renderableManager.setGeometryAt(instance, i,
                                        filament::RenderableManager::PrimitiveType::TRIANGLES,
                                        vertex_buffer_, index_buffer_, 0, 0);
    }
}

void ImGuiRenderer::BuildDrawData() {
    vertices_.clear();
    indices_.clear();
    draw_primitives_.clear();

    const ImDrawData* drawData = ImGui::GetDrawData();
    if (!drawData || drawData->CmdListsCount == 0 || drawData->TotalVtxCount <= 0 ||
        drawData->TotalIdxCount <= 0) {
        return;
    }

    vertices_.reserve(static_cast<size_t>(drawData->TotalVtxCount));
    indices_.reserve(static_cast<size_t>(drawData->TotalIdxCount));
    draw_primitives_.reserve(static_cast<size_t>(drawData->TotalIdxCount / 6) + 8);

    const ImVec2 displayPos = drawData->DisplayPos;

    uint32_t globalVertexOffset = 0;
    for (int listIndex = 0; listIndex < drawData->CmdListsCount; ++listIndex) {
        const ImDrawList* commandList = drawData->CmdLists[listIndex];
        if (!commandList) continue;

        for (const ImDrawVert& srcVertex : commandList->VtxBuffer) {
            Vertex dstVertex;
            dstVertex.position[0] = srcVertex.pos.x - displayPos.x;
            dstVertex.position[1] = srcVertex.pos.y - displayPos.y;
            dstVertex.position[2] = 0.0f;
            dstVertex.uv[0] = srcVertex.uv.x;
            // Filament's texture sampling expects V flipped relative to Dear ImGui atlas UVs.
            dstVertex.uv[1] = 1.0f - srcVertex.uv.y;
            CopyImGuiColorToBytes(srcVertex.col, dstVertex.color);
            vertices_.push_back(dstVertex);
        }

        for (const ImDrawCmd& command : commandList->CmdBuffer) {
            if (command.UserCallback != nullptr) {
                if (command.UserCallback == ImDrawCallback_ResetRenderState) {
                    continue;
                }
                command.UserCallback(commandList, &command);
                continue;
            }

            if (command.ElemCount == 0) continue;

            const float clipMinX = command.ClipRect.x - displayPos.x;
            const float clipMinY = command.ClipRect.y - displayPos.y;
            const float clipMaxX = command.ClipRect.z - displayPos.x;
            const float clipMaxY = command.ClipRect.w - displayPos.y;
            if (clipMaxX <= clipMinX || clipMaxY <= clipMinY) continue;

            DrawPrimitive primitive;
            primitive.index_offset = static_cast<uint32_t>(indices_.size());
            primitive.index_count  = command.ElemCount;
            primitive.texture =
                ResolveTexture(reinterpret_cast<void*>(static_cast<uintptr_t>(command.GetTexID())));
            primitive.clip_min_x   = clipMinX;
            primitive.clip_min_y   = clipMinY;
            primitive.clip_max_x   = clipMaxX;
            primitive.clip_max_y   = clipMaxY;

            const uint32_t baseVertex = globalVertexOffset + command.VtxOffset;
            const ImDrawIdx* sourceIndices = commandList->IdxBuffer.Data + command.IdxOffset;
            for (uint32_t idx = 0; idx < command.ElemCount; ++idx) {
                indices_.push_back(static_cast<uint32_t>(sourceIndices[idx]) + baseVertex);
            }

            draw_primitives_.push_back(primitive);
        }

        globalVertexOffset += static_cast<uint32_t>(commandList->VtxBuffer.Size);
    }

}

void ImGuiRenderer::UpdateCameraProjection() {
    if (!camera_) return;

    camera_->setProjection(filament::Camera::Projection::ORTHO, 0.0,
                           static_cast<double>(window_width_),
                           static_cast<double>(window_height_), 0.0, 0.1, 10.0);
    camera_->lookAt({0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0});
}

void ImGuiRenderer::UpdateInputState() {
    ImGuiIO& io = ImGui::GetIO();
    InputManager& input = InputManager::Get();

    for (KeyBinding& binding : key_bindings_) {
        const bool isDown = input.IsKeyDown(binding.keycode);
        if (isDown == binding.previous_down) continue;
        io.AddKeyEvent(static_cast<ImGuiKey>(binding.keyIndex), isDown);
        binding.previous_down = isDown;
    }

    const bool ctrlDown = input.IsKeyDown(Key::LeftControl) || input.IsKeyDown(Key::RightControl);
    const bool shiftDown = input.IsKeyDown(Key::LeftShift) || input.IsKeyDown(Key::RightShift);
    const bool altDown = input.IsKeyDown(Key::LeftAlt) || input.IsKeyDown(Key::RightAlt);
    const bool superDown = input.IsKeyDown(Key::LeftSuper) || input.IsKeyDown(Key::RightSuper);

    if (ctrlDown != modifier_state_ctrl_) {
        io.AddKeyEvent(ImGuiMod_Ctrl, ctrlDown);
        modifier_state_ctrl_ = ctrlDown;
    }
    if (shiftDown != modifier_state_shift_) {
        io.AddKeyEvent(ImGuiMod_Shift, shiftDown);
        modifier_state_shift_ = shiftDown;
    }
    if (altDown != modifier_state_alt_) {
        io.AddKeyEvent(ImGuiMod_Alt, altDown);
        modifier_state_alt_ = altDown;
    }
    if (superDown != modifier_state_super_) {
        io.AddKeyEvent(ImGuiMod_Super, superDown);
        modifier_state_super_ = superDown;
    }

    static constexpr std::array<MouseButton, 5> kMouseButtons = {
        Mouse::ButtonLeft, Mouse::ButtonRight, Mouse::ButtonMiddle, Mouse::Button3, Mouse::Button4};
    for (size_t i = 0; i < kMouseButtons.size(); ++i) {
        const bool isDown = input.IsMouseButtonDown(kMouseButtons[i]);
        if (isDown == mouse_button_state_[i]) continue;
        io.AddMouseButtonEvent(static_cast<int>(i), isDown);
        mouse_button_state_[i] = isDown;
    }

    bool focused = true;
    if (window_ && window_->GetNativeWindow()) {
        focused = glfwGetWindowAttrib(window_->GetNativeWindow(), GLFW_FOCUSED) == GLFW_TRUE;
    }
    if (focused) {
        const Vector2 mousePosition = input.GetMousePosition();
        if (!has_last_mouse_position_ || mousePosition.x != last_mouse_x_ ||
            mousePosition.y != last_mouse_y_) {
            io.AddMousePosEvent(mousePosition.x, mousePosition.y);
            last_mouse_x_ = mousePosition.x;
            last_mouse_y_ = mousePosition.y;
            has_last_mouse_position_ = true;
        }
    } else if (has_last_mouse_position_) {
        io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);
        has_last_mouse_position_ = false;
    }

    const float mouseWheel = input.GetScrollDelta();
    if (mouseWheel != 0.0f) {
        io.AddMouseWheelEvent(0.0f, mouseWheel);
    }
}

void ImGuiRenderer::FlushQueuedCharacters() {
    if (queued_characters_.empty()) return;
    ImGuiIO& io = ImGui::GetIO();
    for (uint32_t codepoint : queued_characters_) {
        io.AddInputCharacter(codepoint);
    }
    queued_characters_.clear();
}

filament::Texture* ImGuiRenderer::ResolveTexture(void* textureId) const {
    if (!textureId) return font_texture_;
    return reinterpret_cast<filament::Texture*>(textureId);
}

void ImGuiRenderer::OnTextInput(uint32_t codepoint) {
    if (!initialized_) return;
    if (queued_characters_.size() < 1024) {
        queued_characters_.push_back(codepoint);
    }
}

}  // namespace se::ui::imgui
