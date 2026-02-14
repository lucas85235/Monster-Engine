#include "engine/renderer/TextureSystem.h"

#include <filament/Engine.h>
#include <filament/Texture.h>
#include <filament/TextureSampler.h>

#include <spdlog/spdlog.h>

#include <stb_image.h>

#include <cstring>
#include <filesystem>
#include <cmath>

namespace se {

namespace {

uint32_t computeMipLevels(uint32_t width, uint32_t height) {
    return static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
}

} // anonymous namespace

TextureSystem::~TextureSystem() {
    Shutdown();
}

void TextureSystem::Init(filament::Engine* engine) {
    if (engine_) {
        spdlog::warn("TextureSystem::Init called but already initialized. Ignoring.");
        return;
    }

    engine_ = engine;
    CreateDefaultTextures();

    spdlog::info("TextureSystem initialized with default textures.");
}

void TextureSystem::Shutdown() {
    if (!engine_) return;

    cache_.clear();

    for (auto* texture : textures_) {
        engine_->destroy(texture);
    }
    textures_.clear();

    default_white_  = TextureHandle();
    default_normal_ = TextureHandle();
    default_black_  = TextureHandle();

    engine_ = nullptr;
    spdlog::info("TextureSystem shut down.");
}

void TextureSystem::CreateDefaultTextures() {
    // 1x1 white (sRGB)
    {
        uint8_t white[] = {255, 255, 255, 255};
        default_white_ = CreateTexture(white, 1, 1, 4, true, "DefaultWhite");
    }

    // 1x1 flat normal (linear — pointing up: 0.5, 0.5, 1.0 in [0,1] → 128, 128, 255 in [0,255])
    {
        uint8_t normal[] = {128, 128, 255, 255};
        default_normal_ = CreateTexture(normal, 1, 1, 4, false, "DefaultNormal");
    }

    // 1x1 black (linear — for AO/emissive fallback)
    {
        uint8_t black[] = {0, 0, 0, 255};
        default_black_ = CreateTexture(black, 1, 1, 4, false, "DefaultBlack");
    }
}

TextureHandle TextureSystem::LoadTexture(const std::string& path, bool sRGB) {
    if (!engine_) {
        spdlog::error("TextureSystem::LoadTexture called before Init()!");
        return TextureHandle();
    }

    // Resolve to canonical path for cache key
    std::string canonicalPath;
    try {
        if (std::filesystem::exists(path)) {
            canonicalPath = std::filesystem::canonical(path).string();
        } else {
            spdlog::error("TextureSystem: File not found: {}", path);
            return sRGB ? default_white_ : default_normal_;
        }
    } catch (const std::exception& e) {
        spdlog::error("TextureSystem: Path error for '{}': {}", path, e.what());
        return sRGB ? default_white_ : default_normal_;
    }

    // Check cache
    auto it = cache_.find(canonicalPath);
    if (it != cache_.end()) {
        return it->second;
    }

    // Load image with stb_image
    int width, height, channels;
    stbi_set_flip_vertically_on_load(false); // Filament expects top-down
    uint8_t* data = stbi_load(canonicalPath.c_str(), &width, &height, &channels, 4); // Force RGBA

    if (!data) {
        spdlog::error("TextureSystem: Failed to load image '{}': {}", canonicalPath, stbi_failure_reason());
        return sRGB ? default_white_ : default_normal_;
    }

    TextureHandle handle = CreateTexture(data, static_cast<uint32_t>(width),
                                          static_cast<uint32_t>(height), 4, sRGB,
                                          std::filesystem::path(canonicalPath).filename().string());
    stbi_image_free(data);

    if (handle.IsValid()) {
        cache_[canonicalPath] = handle;
        spdlog::info("TextureSystem: Loaded texture '{}' ({}x{}, {})",
                     std::filesystem::path(canonicalPath).filename().string(),
                     width, height, sRGB ? "sRGB" : "linear");
    }

    return handle;
}

TextureHandle TextureSystem::CreateTexture(const uint8_t* data, uint32_t width, uint32_t height,
                                            uint32_t channels, bool sRGB,
                                            const std::string& name) {
    if (!engine_) {
        spdlog::error("TextureSystem::CreateTexture called before Init()!");
        return TextureHandle();
    }

    uint32_t mipLevels = computeMipLevels(width, height);

    // Choose the appropriate Filament internal format
    filament::Texture::InternalFormat format;
    if (sRGB) {
        format = filament::Texture::InternalFormat::SRGB8_A8;
    } else {
        format = filament::Texture::InternalFormat::RGBA8;
    }

    auto* texture = filament::Texture::Builder()
        .width(width)
        .height(height)
        .levels(static_cast<uint8_t>(mipLevels))
        .format(format)
        .sampler(filament::Texture::Sampler::SAMPLER_2D)
        .build(*engine_);

    if (!texture) {
        spdlog::error("TextureSystem: Failed to create Filament texture '{}'", name);
        return TextureHandle();
    }

    // Upload pixel data (always RGBA8, 4 bytes per pixel)
    size_t dataSize = width * height * 4;
    auto* pixelCopy = new uint8_t[dataSize];
    std::memcpy(pixelCopy, data, dataSize);

    filament::Texture::PixelBufferDescriptor buffer(
        pixelCopy, dataSize,
        filament::Texture::Format::RGBA,
        filament::Texture::Type::UBYTE,
        [](void* buf, size_t, void*) { delete[] static_cast<uint8_t*>(buf); }
    );
    texture->setImage(*engine_, 0, std::move(buffer));

    // Generate mipmaps for filtering quality
    texture->generateMipmaps(*engine_);

    textures_.push_back(texture);

    spdlog::debug("TextureSystem: Created texture '{}' ({}x{}, {} mip levels)",
                  name, width, height, mipLevels);

    return TextureHandle(texture, width, height);
}

TextureHandle TextureSystem::GetDefaultWhite() {
    return default_white_;
}

TextureHandle TextureSystem::GetDefaultNormal() {
    return default_normal_;
}

TextureHandle TextureSystem::GetDefaultBlack() {
    return default_black_;
}

} // namespace se
