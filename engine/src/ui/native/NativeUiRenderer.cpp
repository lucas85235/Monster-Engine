#include "engine/ui/native/NativeUiRenderer.h"

#include "engine/Log.h"
#include "engine/renderer/FilamentContext.h"
#include "engine/renderer/FilamentRenderer.h"

#include <filament/Camera.h>
#include <filament/Engine.h>
#include <filament/IndexBuffer.h>
#include <filament/Material.h>
#include <filament/MaterialInstance.h>
#include <filament/RenderableManager.h>
#include <filament/Scene.h>
#include <filament/Texture.h>
#include <filament/TextureSampler.h>
#include <filament/TransformManager.h>
#include <filament/VertexBuffer.h>
#include <filament/View.h>
#include <filament/Viewport.h>

#include <filamat/MaterialBuilder.h>

#include <imgui.h>

#include <math/mat4.h>
#include <math/vec3.h>

#include <utils/EntityManager.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <limits>
#include <system_error>

namespace se::ui {

namespace {

constexpr uint8_t kUiLayerMask = 0x80;  // layer 7
constexpr const char* kUiFontPath = "assets/fonts/Roboto-Regular.ttf";
constexpr ImWchar kUiLatin1GlyphRanges[] = {0x0020, 0x00FF, 0};

constexpr const char* kUiMaterialSource = R"FILAMENT(
void material(inout MaterialInputs material) {
    prepareMaterial(material);
    float4 atlas = texture(materialParams_uiAtlas, getUV0());
    // Be tolerant to backend texture swizzle differences.
    float coverage = max(max(atlas.r, atlas.g), max(atlas.b, atlas.a));
    float4 color = getColor();
    // Filament TRANSPARENT expects premultiplied alpha.
    float alpha = color.a * coverage;
    material.baseColor = float4(color.rgb * alpha, alpha);
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

uint32_t DecodeUtf8Codepoint(const char*& cursor, const char* end) {
    if (cursor >= end) return 0;

    const unsigned char first = static_cast<unsigned char>(*cursor++);
    if (first < 0x80) return first;

    // 2-byte sequence
    if ((first & 0xE0u) == 0xC0u) {
        if (cursor >= end) return static_cast<uint32_t>('?');
        const unsigned char c1 = static_cast<unsigned char>(*cursor++);
        if ((c1 & 0xC0u) != 0x80u) return static_cast<uint32_t>('?');
        return ((first & 0x1Fu) << 6) | (c1 & 0x3Fu);
    }

    // 3-byte sequence
    if ((first & 0xF0u) == 0xE0u) {
        if ((end - cursor) < 2) {
            cursor = end;
            return static_cast<uint32_t>('?');
        }
        const unsigned char c1 = static_cast<unsigned char>(*cursor++);
        const unsigned char c2 = static_cast<unsigned char>(*cursor++);
        if ((c1 & 0xC0u) != 0x80u || (c2 & 0xC0u) != 0x80u) return static_cast<uint32_t>('?');
        return ((first & 0x0Fu) << 12) | ((c1 & 0x3Fu) << 6) | (c2 & 0x3Fu);
    }

    // 4-byte sequence
    if ((first & 0xF8u) == 0xF0u) {
        if ((end - cursor) < 3) {
            cursor = end;
            return static_cast<uint32_t>('?');
        }
        const unsigned char c1 = static_cast<unsigned char>(*cursor++);
        const unsigned char c2 = static_cast<unsigned char>(*cursor++);
        const unsigned char c3 = static_cast<unsigned char>(*cursor++);
        if ((c1 & 0xC0u) != 0x80u || (c2 & 0xC0u) != 0x80u || (c3 & 0xC0u) != 0x80u) {
            return static_cast<uint32_t>('?');
        }
        return ((first & 0x07u) << 18) | ((c1 & 0x3Fu) << 12) | ((c2 & 0x3Fu) << 6) |
               (c3 & 0x3Fu);
    }

    return static_cast<uint32_t>('?');
}

ImWchar ToUiGlyphCodepoint(uint32_t codepoint) {
    // Native UI text rendering supports Latin-1 to cover common Western locales.
    if (codepoint >= 0x0020u && codepoint <= 0x00FFu) {
        return static_cast<ImWchar>(codepoint);
    }
    return static_cast<ImWchar>('?');
}

}  // namespace

NativeUiRenderer& NativeUiRenderer::Get() {
    static NativeUiRenderer instance;
    return instance;
}

void NativeUiRenderer::Init(FilamentContext* context, FilamentRenderer* renderer, uint32_t width,
                            uint32_t height) {
    if (initialized_) return;
    if (!context || !renderer) {
        SE_LOG_ERROR("NativeUiRenderer::Init called with null context/renderer.");
        return;
    }

    context_         = context;
    renderer_        = renderer;
    // At Init time we only receive one size; assume it's the window size (screen points).
    // The framebuffer size will be corrected on the first OnResize from the main loop.
    viewport_width_      = std::max<uint32_t>(1, width);
    viewport_height_     = std::max<uint32_t>(1, height);
    framebuffer_width_   = viewport_width_;
    framebuffer_height_  = viewport_height_;

    CreateResources();
    if (!initialized_) {
        DestroyResources();
        context_  = nullptr;
        renderer_ = nullptr;
    }
}

void NativeUiRenderer::Shutdown() {
    if (!initialized_ && !scene_ && !view_ && !material_ && !atlas_texture_ && !font_atlas_) {
        return;
    }
    DestroyResources();
    context_  = nullptr;
    renderer_ = nullptr;
}

void NativeUiRenderer::OnResize(uint32_t framebufferWidth, uint32_t framebufferHeight,
                                uint32_t windowWidth, uint32_t windowHeight) {
    framebuffer_width_  = std::max<uint32_t>(1, framebufferWidth);
    framebuffer_height_ = std::max<uint32_t>(1, framebufferHeight);
    viewport_width_     = std::max<uint32_t>(1, windowWidth);
    viewport_height_    = std::max<uint32_t>(1, windowHeight);

    if (!initialized_) return;

    // Screen-space UI geometry must be regenerated for the new viewport.
    InvalidateRetainedGeometry();

    if (view_) {
        // Filament viewport must be in framebuffer pixels.
        view_->setViewport({0, 0, framebuffer_width_, framebuffer_height_});
    }
    // Ortho projection uses screen points so UI coordinates match mouse coords.
    UpdateCameraProjection();

    auto* engine = context_ ? context_->GetEngine() : nullptr;
    if (engine && has_renderable_entity_) {
        auto& rm = engine->getRenderableManager();
        auto inst = rm.getInstance(renderable_entity_);
        if (inst.isValid()) {
            // AABB in framebuffer pixel space (must cover the full viewport).
            rm.setAxisAlignedBoundingBox(inst,
                                         {{framebuffer_width_ * 0.5f, framebuffer_height_ * 0.5f, 0.0f},
                                          {framebuffer_width_ * 0.5f, framebuffer_height_ * 0.5f, 1.0f}});
        }
    }
}

void NativeUiRenderer::BeginFrame() {
    if (!initialized_) return;

    last_frame_stats_ = FrameStats{};
    frame_quads_submitted_ = 0;
    frame_reuse_active_ = false;

    if (retained_geometry_reuse_enabled_ && retain_previous_geometry_ && has_geometry_) {
        last_frame_stats_.geometryReused = true;
        frame_reuse_active_ = true;
        retain_previous_geometry_ = false;
        return;
    }
    retain_previous_geometry_ = false;

    if (!vertices_.empty() || !indices_.empty()) {
        vertices_.clear();
        indices_.clear();
        geometry_dirty_ = true;
    }
}

void NativeUiRenderer::EndFrame() {
    if (!initialized_) return;
    last_frame_stats_.quadsSubmitted = frame_quads_submitted_;
    if (!geometry_dirty_) return;
    UploadGeometry();
    geometry_dirty_ = false;

    static int uploadDebugFrames = 0;
    if (uploadDebugFrames < 5) {
        std::cout << "NativeUiRenderer::EndFrame uploaded quads=" << last_frame_stats_.quadsSubmitted
                  << " draw_calls=" << last_frame_stats_.drawCallsIssued
                  << " vertices=" << last_frame_stats_.verticesUploaded
                  << " indices=" << last_frame_stats_.indicesUploaded << std::endl;
        uploadDebugFrames++;
    }
}

void NativeUiRenderer::RetainPreviousFrameGeometry() {
    if (!initialized_) return;
    retain_previous_geometry_ = true;
}

void NativeUiRenderer::InvalidateRetainedGeometry() {
    retain_previous_geometry_ = false;
    frame_reuse_active_       = false;
    vertices_.clear();
    indices_.clear();
    geometry_dirty_ = true;
    has_geometry_   = false;
}

void NativeUiRenderer::DrawFilledRect(float x, float y, float width, float height,
                                      const NativeUiColor& color) {
    if (!initialized_ || width <= 0.0f || height <= 0.0f) return;
    PrepareForDraw();
    PushQuad(x, y, x + width, y + height, white_uv_x_, white_uv_y_, white_uv_x_, white_uv_y_,
             color);
}

void NativeUiRenderer::DrawRect(float x, float y, float width, float height, float thickness,
                                const NativeUiColor& color) {
    if (!initialized_ || width <= 0.0f || height <= 0.0f || thickness <= 0.0f) return;
    geometry_dirty_ = true;
    const float t = std::min(thickness, std::min(width, height) * 0.5f);
    DrawFilledRect(x, y, width, t, color);
    DrawFilledRect(x, y + height - t, width, t, color);
    DrawFilledRect(x, y + t, t, std::max(0.0f, height - 2.0f * t), color);
    DrawFilledRect(x + width - t, y + t, t, std::max(0.0f, height - 2.0f * t), color);
}

void NativeUiRenderer::DrawText(std::string_view text, float x, float y, const NativeUiColor& color,
                                float scale) {
    if (!initialized_ || !font_ || text.empty() || scale <= 0.0f) return;
    PrepareForDraw();

    ImFontBaked* baked = font_->GetFontBaked(font_base_size_);
    if (!baked) return;

    float penX     = x;
    float baseline = y + baked->Ascent * scale;
    const char* cursor = text.data();
    const char* end    = text.data() + text.size();

    while (cursor < end) {
        const uint32_t codepoint = DecodeUtf8Codepoint(cursor, end);
        if (codepoint == '\r') continue;
        if (codepoint == '\n') {
            penX     = x;
            baseline += baked->Size * scale;
            continue;
        }
        if (codepoint < 32u) continue;

        const ImWchar glyphCode = ToUiGlyphCodepoint(codepoint);
        const ImFontGlyph* glyph = baked->FindGlyph(glyphCode);
        if (!glyph) continue;
        if (!glyph->Visible) {
            penX += glyph->AdvanceX * scale;
            continue;
        }

        const float x0 = penX + glyph->X0 * scale;
        const float y0 = baseline + glyph->Y0 * scale;
        const float x1 = penX + glyph->X1 * scale;
        const float y1 = baseline + glyph->Y1 * scale;
        PushQuad(x0, y0, x1, y1, glyph->U0, glyph->V0, glyph->U1, glyph->V1, color);

        penX += glyph->AdvanceX * scale;
    }
}

float NativeUiRenderer::GetLineHeight(float scale) const {
    if (!font_ || scale <= 0.0f) return 0.0f;
    ImFontBaked* baked = font_->GetFontBaked(font_base_size_);
    return baked ? baked->Size * scale : (font_base_size_ * scale);
}

float NativeUiRenderer::MeasureTextWidth(std::string_view text, float scale) const {
    if (!font_ || text.empty() || scale <= 0.0f) return 0.0f;

    ImFontBaked* baked = font_->GetFontBaked(font_base_size_);
    if (!baked) return 0.0f;

    float width = 0.0f;
    const char* cursor = text.data();
    const char* end    = text.data() + text.size();
    while (cursor < end) {
        const uint32_t codepoint = DecodeUtf8Codepoint(cursor, end);
        if (codepoint == '\n') break;
        if (codepoint < 32u) continue;

        const ImWchar glyphCode = ToUiGlyphCodepoint(codepoint);
        const ImFontGlyph* glyph = baked->FindGlyph(glyphCode);
        if (!glyph) continue;
        width += glyph->AdvanceX * scale;
    }

    return width;
}

void NativeUiRenderer::CreateResources() {
    auto* engine = context_ ? context_->GetEngine() : nullptr;
    if (!engine || !renderer_) {
        SE_LOG_ERROR("NativeUiRenderer failed to initialize: missing engine/renderer.");
        return;
    }

    std::error_code cwdError;
    const auto cwd = std::filesystem::current_path(cwdError);
    if (cwdError) {
        std::cerr << "NativeUiRenderer font pipeline v2 active. cwd='<unavailable>' (error='"
                  << cwdError.message() << "')" << std::endl;
    } else {
        std::cout << "NativeUiRenderer font pipeline v2 active. cwd='" << cwd.string() << "'"
                  << std::endl;
    }

    font_atlas_ = new ImFontAtlas();
    const std::array<std::filesystem::path, 2> fontCandidates = {
        std::filesystem::path(kUiFontPath), std::filesystem::path("..") / kUiFontPath};
    for (const auto& candidate : fontCandidates) {
        const std::string fontPath = candidate.string();
        font_ = font_atlas_->AddFontFromFileTTF(fontPath.c_str(), font_base_size_, nullptr,
                                                kUiLatin1GlyphRanges);
        if (font_) {
            std::cout << "NativeUiRenderer loaded UI font '" << fontPath << "'" << std::endl;
            break;
        }
    }

    if (!font_) {
        std::cerr << "NativeUiRenderer using Dear ImGui default font fallback." << std::endl;
        ImFontConfig fallbackConfig;
        fallbackConfig.GlyphRanges = kUiLatin1GlyphRanges;
        font_ = font_atlas_->AddFontDefault(&fallbackConfig);
        if (!font_) {
            font_ = font_atlas_->AddFontDefault();
        }
    }
    if (font_ && font_->LegacySize > 0.0f) {
        font_base_size_ = font_->LegacySize;
    }

    // Build the atlas first so the base layout is established.
    font_atlas_->Build();

    // Warm-up Latin-1 AFTER Build() so on-demand glyph baking
    // adds glyphs into the already-built atlas layout.
    if (font_) {
        if (ImFontBaked* baked = font_->GetFontBaked(font_base_size_)) {
            for (ImWchar cp = 0x0020; cp <= 0x00FF; ++cp) {
                (void)baked->FindGlyph(cp);
            }
            (void)baked->FindGlyph('?');
        }
    }

    // Retrieve final pixel data after warmup may have expanded the atlas.
    unsigned char* atlasPixels = nullptr;
    int atlasWidth             = 0;
    int atlasHeight            = 0;
    font_atlas_->GetTexDataAsAlpha8(&atlasPixels, &atlasWidth, &atlasHeight);

    static const unsigned char kFallbackAtlasPixel = 255u;
    if (!atlasPixels || atlasWidth <= 0 || atlasHeight <= 0) {
        std::cerr
            << "NativeUiRenderer failed to build font atlas; using 1x1 white fallback atlas."
            << std::endl;
        atlasPixels = const_cast<unsigned char*>(&kFallbackAtlasPixel);
        atlasWidth  = 1;
        atlasHeight = 1;
        white_uv_x_ = 0.5f;
        white_uv_y_ = 0.5f;
    } else {
        white_uv_x_ = font_atlas_->TexUvWhitePixel.x;
        white_uv_y_ = font_atlas_->TexUvWhitePixel.y;
    }

    if (!font_) {
        std::cerr << "NativeUiRenderer text rendering disabled (font unavailable)." << std::endl;
    }

    atlas_texture_ = filament::Texture::Builder()
                         .width(static_cast<uint32_t>(atlasWidth))
                         .height(static_cast<uint32_t>(atlasHeight))
                         .levels(1)
                         .sampler(filament::Texture::Sampler::SAMPLER_2D)
                         .format(filament::Texture::InternalFormat::RGBA8)
                         .build(*engine);

    if (!atlas_texture_) {
        SE_LOG_ERROR("NativeUiRenderer failed to create atlas texture.");
        delete font_atlas_;
        font_atlas_ = nullptr;
        font_       = nullptr;
        return;
    }

    const size_t atlasPixelsCount =
        static_cast<size_t>(atlasWidth) * static_cast<size_t>(atlasHeight);
    const size_t atlasBytes = atlasPixelsCount * 4;
    auto* atlasCopy = new uint8_t[atlasBytes];
    for (size_t i = 0; i < atlasPixelsCount; ++i) {
        const uint8_t coverage      = atlasPixels[i];
        const size_t  base          = i * 4;
        atlasCopy[base + 0] = coverage;
        atlasCopy[base + 1] = coverage;
        atlasCopy[base + 2] = coverage;
        atlasCopy[base + 3] = coverage;
    }

    const int whiteX = std::clamp(static_cast<int>(white_uv_x_ * static_cast<float>(atlasWidth)),
                                  0, atlasWidth - 1);
    const int whiteY = std::clamp(static_cast<int>(white_uv_y_ * static_cast<float>(atlasHeight)),
                                  0, atlasHeight - 1);
    const size_t whiteBase =
        (static_cast<size_t>(whiteY) * static_cast<size_t>(atlasWidth) +
         static_cast<size_t>(whiteX)) *
        4;
    atlasCopy[whiteBase + 0] = 255;
    atlasCopy[whiteBase + 1] = 255;
    atlasCopy[whiteBase + 2] = 255;
    atlasCopy[whiteBase + 3] = 255;

    atlas_texture_->setImage(*engine, 0,
                             filament::Texture::PixelBufferDescriptor(
                                 atlasCopy, atlasBytes, filament::Texture::Format::RGBA,
                                 filament::Texture::Type::UBYTE,
                                 [](void* data, size_t, void*) { delete[] static_cast<uint8_t*>(data); }));

    filamat::MaterialBuilder builder;
    builder.name("NativeUiMaterial")
        .material(kUiMaterialSource)
        .shading(filament::Shading::UNLIT)
        .require(filament::VertexAttribute::UV0)
        .require(filament::VertexAttribute::COLOR)
        .parameter("uiAtlas", filamat::MaterialBuilder::SamplerType::SAMPLER_2D,
                   filamat::MaterialBuilder::SamplerFormat::FLOAT)
        .blending(filamat::MaterialBuilder::BlendingMode::TRANSPARENT)
        .depthWrite(false)
        .depthCulling(false)
        .doubleSided(true)
        .targetApi(filamat::MaterialBuilder::TargetApi::ALL)
        .platform(filamat::MaterialBuilder::Platform::ALL);

    filamat::Package package = builder.build(engine->getJobSystem());
    if (!package.isValid()) {
        SE_LOG_ERROR("NativeUiRenderer failed to compile UI material package.");
        return;
    }

    material_ = filament::Material::Builder().package(package.getData(), package.getSize()).build(*engine);
    if (!material_) {
        SE_LOG_ERROR("NativeUiRenderer failed to create material.");
        return;
    }

    material_instance_ = material_->createInstance("NativeUiMaterialInstance");
    if (!material_instance_) {
        SE_LOG_ERROR("NativeUiRenderer failed to create material instance.");
        return;
    }

    filament::TextureSampler atlasSampler(filament::TextureSampler::MinFilter::LINEAR,
                                          filament::TextureSampler::MagFilter::LINEAR,
                                          filament::TextureSampler::WrapMode::CLAMP_TO_EDGE);
    material_instance_->setParameter("uiAtlas", atlas_texture_, atlasSampler);

    scene_ = engine->createScene();
    view_  = engine->createView();
    if (!scene_ || !view_) {
        SE_LOG_ERROR("NativeUiRenderer failed to create Filament scene/view.");
        return;
    }

    camera_entity_     = utils::EntityManager::get().create();
    has_camera_entity_ = true;
    camera_            = engine->createCamera(camera_entity_);
    if (!camera_) {
        SE_LOG_ERROR("NativeUiRenderer failed to create camera.");
        return;
    }

    view_->setScene(scene_);
    view_->setCamera(camera_);
    view_->setViewport({0, 0, framebuffer_width_, framebuffer_height_});
    view_->setBlendMode(filament::View::BlendMode::TRANSLUCENT);
    view_->setVisibleLayers(0xFF, kUiLayerMask);
    view_->setPostProcessingEnabled(false);
    view_->setFrustumCullingEnabled(false);
    view_->setAntiAliasing(filament::AntiAliasing::NONE);
    view_->setDithering(filament::Dithering::NONE);
    UpdateCameraProjection();

    RecreateGeometryBuffers(4096, 6144);

    renderer_->RegisterOverlayView(view_);

    initialized_ = true;
    geometry_dirty_ = true;
    has_geometry_   = false;
    std::cout << "NativeUiRenderer initialized (Filament overlay view)." << std::endl;
}

void NativeUiRenderer::DestroyResources() {
    auto* engine = context_ ? context_->GetEngine() : nullptr;

    if (renderer_ && view_) {
        renderer_->UnregisterOverlayView(view_);
    }

    if (engine && has_renderable_entity_) {
        if (scene_) {
            scene_->remove(renderable_entity_);
        }
        engine->destroy(renderable_entity_);
        utils::EntityManager::get().destroy(renderable_entity_);
        has_renderable_entity_ = false;
    } else {
        has_renderable_entity_ = false;
    }

    if (engine && vertex_buffer_) {
        engine->destroy(vertex_buffer_);
        vertex_buffer_ = nullptr;
    }
    if (engine && index_buffer_) {
        engine->destroy(index_buffer_);
        index_buffer_ = nullptr;
    }
    if (!engine) {
        vertex_buffer_ = nullptr;
        index_buffer_  = nullptr;
    }

    if (engine && material_instance_) {
        engine->destroy(material_instance_);
        material_instance_ = nullptr;
    }
    if (engine && material_) {
        engine->destroy(material_);
        material_ = nullptr;
    }
    if (engine && atlas_texture_) {
        engine->destroy(atlas_texture_);
        atlas_texture_ = nullptr;
    }
    if (!engine) {
        material_instance_ = nullptr;
        material_          = nullptr;
        atlas_texture_     = nullptr;
    }

    if (engine && view_) {
        engine->destroy(view_);
        view_ = nullptr;
    }
    if (engine && scene_) {
        engine->destroy(scene_);
        scene_ = nullptr;
    }
    if (!engine) {
        view_  = nullptr;
        scene_ = nullptr;
    }

    if (has_camera_entity_) {
        if (engine && camera_) {
            engine->destroyCameraComponent(camera_entity_);
            camera_ = nullptr;
        }
        utils::EntityManager::get().destroy(camera_entity_);
        has_camera_entity_ = false;
    } else {
        camera_ = nullptr;
    }

    delete font_atlas_;
    font_atlas_ = nullptr;
    font_       = nullptr;

    vertices_.clear();
    indices_.clear();
    vertex_capacity_ = 0;
    index_capacity_  = 0;
    initialized_     = false;
    retain_previous_geometry_ = false;
    frame_reuse_active_       = false;
    geometry_dirty_           = true;
    has_geometry_             = false;
    frame_quads_submitted_    = 0;
    last_frame_stats_         = FrameStats{};

    SE_LOG_INFO("NativeUiRenderer shut down.");
}

void NativeUiRenderer::RecreateGeometryBuffers(size_t requiredVertexCount, size_t requiredIndexCount) {
    auto* engine = context_ ? context_->GetEngine() : nullptr;
    if (!engine) return;

    vertex_capacity_ = NextPow2(std::max<size_t>(requiredVertexCount, 6));
    index_capacity_  = NextPow2(std::max<size_t>(requiredIndexCount, 6));

    if (has_renderable_entity_) {
        if (scene_) {
            scene_->remove(renderable_entity_);
        }
        engine->destroy(renderable_entity_);
        utils::EntityManager::get().destroy(renderable_entity_);
        has_renderable_entity_ = false;
    }

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
                                    offsetof(UiVertex, position), sizeof(UiVertex))
                         .attribute(filament::VertexAttribute::UV0, 0,
                                    filament::VertexBuffer::AttributeType::FLOAT2,
                                    offsetof(UiVertex, uv), sizeof(UiVertex))
                         .attribute(filament::VertexAttribute::COLOR, 0,
                                    filament::VertexBuffer::AttributeType::UBYTE4,
                                    offsetof(UiVertex, color), sizeof(UiVertex))
                         .normalized(filament::VertexAttribute::COLOR)
                         .build(*engine);

    index_buffer_ = filament::IndexBuffer::Builder()
                        .indexCount(static_cast<uint32_t>(index_capacity_))
                        .bufferType(filament::IndexBuffer::IndexType::UINT)
                        .build(*engine);

    if (!vertex_buffer_ || !index_buffer_ || !material_instance_) {
        SE_LOG_ERROR("NativeUiRenderer failed to recreate geometry buffers.");
        return;
    }

    renderable_entity_     = utils::EntityManager::get().create();
    has_renderable_entity_ = true;

    filament::RenderableManager::Builder(1)
        .geometry(0, filament::RenderableManager::PrimitiveType::TRIANGLES, vertex_buffer_,
                  index_buffer_, 0, 0)
        .material(0, material_instance_)
        .boundingBox({{framebuffer_width_ * 0.5f, framebuffer_height_ * 0.5f, 0.0f},
                      {framebuffer_width_ * 0.5f, framebuffer_height_ * 0.5f, 1.0f}})
        .culling(false)
        .castShadows(false)
        .receiveShadows(false)
        .priority(7)
        .layerMask(0xFF, kUiLayerMask)
        .build(*engine, renderable_entity_);

    if (scene_) {
        scene_->addEntity(renderable_entity_);
    }
}

void NativeUiRenderer::UploadGeometry() {
    auto* engine = context_ ? context_->GetEngine() : nullptr;
    if (!engine || !vertex_buffer_ || !index_buffer_ || !has_renderable_entity_) return;

    last_frame_stats_.drawCallsIssued  = 0;
    last_frame_stats_.verticesUploaded = 0;
    last_frame_stats_.indicesUploaded  = 0;
    last_frame_stats_.geometryUploaded = false;

    if (vertices_.size() > vertex_capacity_ || indices_.size() > index_capacity_) {
        RecreateGeometryBuffers(vertices_.size(), indices_.size());
        if (!vertex_buffer_ || !index_buffer_ || !has_renderable_entity_) return;
    }

    if (!vertices_.empty()) {
        auto* vertexCopy = new UiVertex[vertices_.size()];
        std::memcpy(vertexCopy, vertices_.data(), vertices_.size() * sizeof(UiVertex));
        vertex_buffer_->setBufferAt(
            *engine, 0,
            filament::VertexBuffer::BufferDescriptor(
                vertexCopy, vertices_.size() * sizeof(UiVertex),
                [](void* data, size_t, void*) { delete[] static_cast<UiVertex*>(data); }));
        last_frame_stats_.verticesUploaded = static_cast<uint32_t>(vertices_.size());
        last_frame_stats_.geometryUploaded = true;
    }

    if (!indices_.empty()) {
        auto* indexCopy = new uint32_t[indices_.size()];
        std::memcpy(indexCopy, indices_.data(), indices_.size() * sizeof(uint32_t));
        index_buffer_->setBuffer(
            *engine,
            filament::IndexBuffer::BufferDescriptor(
                indexCopy, indices_.size() * sizeof(uint32_t),
                [](void* data, size_t, void*) { delete[] static_cast<uint32_t*>(data); }));
        last_frame_stats_.indicesUploaded = static_cast<uint32_t>(indices_.size());
        last_frame_stats_.geometryUploaded = true;
    }

    static int vertexDebugFrames = 0;
    if (vertexDebugFrames < 5 && !vertices_.empty() && !indices_.empty()) {
        const UiVertex& v = vertices_.front();
        std::cout << "NativeUiRenderer::UploadGeometry v0 pos=(" << v.position[0] << ","
                  << v.position[1] << "," << v.position[2] << ") uv=(" << v.uv[0] << ","
                  << v.uv[1] << ") color=(" << static_cast<int>(v.color[0]) << ","
                  << static_cast<int>(v.color[1]) << "," << static_cast<int>(v.color[2]) << ","
                  << static_cast<int>(v.color[3]) << ") viewport=(" << viewport_width_ << "x"
                  << viewport_height_ << ") framebuffer=(" << framebuffer_width_ << "x"
                  << framebuffer_height_ << ") flip_uv_v=" << (flip_uv_v_ ? 1 : 0)
                  << std::endl;
        vertexDebugFrames++;
    }

    auto& rm = engine->getRenderableManager();
    auto inst = rm.getInstance(renderable_entity_);
    if (!inst.isValid()) return;

    rm.setGeometryAt(inst, 0, filament::RenderableManager::PrimitiveType::TRIANGLES, vertex_buffer_,
                     index_buffer_, 0, indices_.size());
    rm.setLayerMask(inst, 0xFF, indices_.empty() ? 0x00 : kUiLayerMask);
    has_geometry_ = !indices_.empty();
    if (!indices_.empty()) {
        last_frame_stats_.drawCallsIssued = 1;
    }
}

void NativeUiRenderer::UpdateCameraProjection() {
    if (!camera_) return;

    camera_->setProjection(filament::Camera::Projection::ORTHO, 0.0, static_cast<double>(viewport_width_),
                           static_cast<double>(viewport_height_), 0.0, 0.1, 10.0);
    camera_->lookAt({0.0, 0.0, 1.0}, {0.0, 0.0, 0.0}, {0.0, 1.0, 0.0});
}

void NativeUiRenderer::PrepareForDraw() {
    if (frame_reuse_active_) {
        // A retained pass requested "reuse", but someone emitted new UI this frame.
        // Drop the previous geometry so this frame can upload only the fresh commands.
        vertices_.clear();
        indices_.clear();
        frame_reuse_active_ = false;
        last_frame_stats_.geometryReused = false;
    }
    geometry_dirty_ = true;
}

void NativeUiRenderer::PushQuad(float x0, float y0, float x1, float y1, float u0, float v0,
                                float u1, float v1, const NativeUiColor& color) {
    if (x1 <= x0 || y1 <= y0) return;
    if (vertices_.size() + 4 > static_cast<size_t>(std::numeric_limits<uint32_t>::max())) return;

    const uint8_t cr = ToByte(color.r);
    const uint8_t cg = ToByte(color.g);
    const uint8_t cb = ToByte(color.b);
    const uint8_t ca = ToByte(color.a);

    const uint32_t base = static_cast<uint32_t>(vertices_.size());
    const float finalV0 = flip_uv_v_ ? (1.0f - v0) : v0;
    const float finalV1 = flip_uv_v_ ? (1.0f - v1) : v1;

    UiVertex v0s;
    v0s.position[0] = x0;
    v0s.position[1] = y0;
    v0s.position[2] = 0.0f;
    v0s.uv[0]       = u0;
    v0s.uv[1]       = finalV0;
    v0s.color[0]    = cr;
    v0s.color[1]    = cg;
    v0s.color[2]    = cb;
    v0s.color[3]    = ca;

    UiVertex v1s = v0s;
    v1s.position[0] = x1;
    v1s.uv[0]       = u1;

    UiVertex v2s = v0s;
    v2s.position[0] = x1;
    v2s.position[1] = y1;
    v2s.uv[0]       = u1;
    v2s.uv[1]       = finalV1;

    UiVertex v3s = v0s;
    v3s.position[1] = y1;
    v3s.uv[1]       = finalV1;

    vertices_.push_back(v0s);
    vertices_.push_back(v1s);
    vertices_.push_back(v2s);
    vertices_.push_back(v3s);

    indices_.push_back(base + 0);
    indices_.push_back(base + 1);
    indices_.push_back(base + 2);
    indices_.push_back(base + 0);
    indices_.push_back(base + 2);
    indices_.push_back(base + 3);
    frame_quads_submitted_++;
}

uint8_t NativeUiRenderer::ToByte(float value) {
    const float clamped = std::clamp(value, 0.0f, 1.0f);
    return static_cast<uint8_t>(std::lround(clamped * 255.0f));
}

}  // namespace se::ui
