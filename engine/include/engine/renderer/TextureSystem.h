#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "engine/renderer/TextureHandle.h"

namespace filament {
class Engine;
class Texture;
} // namespace filament

namespace se {

/**
 * Manages Filament Texture lifetimes and loading.
 *
 * Provides image loading from disk via stb_image, with caching by file path.
 * Also provides default fallback textures (1x1 white, 1x1 flat normal).
 *
 * All Textures are owned by this system and destroyed on Shutdown().
 */
class TextureSystem {
public:
    TextureSystem() = default;
    ~TextureSystem();

    // Non-copyable
    TextureSystem(const TextureSystem&) = delete;
    TextureSystem& operator=(const TextureSystem&) = delete;

    /**
     * Initialize with a Filament engine. Must be called before any other method.
     */
    void Init(filament::Engine* engine);

    /**
     * Shutdown and destroy all textures.
     */
    void Shutdown();

    /**
     * Load a texture from an image file on disk.
     * Supports PNG, JPG, TGA, BMP, HDR via stb_image.
     *
     * @param path  Path to the image file.
     * @param sRGB  If true, treat as sRGB (base color, emissive). If false, linear (normal, metallic, roughness, AO).
     * @return Handle to the loaded texture. Cached by canonical path — repeat calls return the same handle.
     */
    TextureHandle LoadTexture(const std::string& path, bool sRGB = true);

    /**
     * Create a texture from raw pixel data in memory.
     *
     * @param data     Raw RGBA pixel data.
     * @param width    Image width in pixels.
     * @param height   Image height in pixels.
     * @param channels Number of channels (3 = RGB, 4 = RGBA).
     * @param sRGB     If true, treat as sRGB color space.
     * @param name     Debug name for this texture.
     * @return Handle to the created texture.
     */
    TextureHandle CreateTexture(const uint8_t* data, uint32_t width, uint32_t height,
                                uint32_t channels, bool sRGB = true,
                                const std::string& name = "");

    /**
     * Get a default 1x1 white texture (for missing base color maps).
     */
    TextureHandle GetDefaultWhite();

    /**
     * Get a default 1x1 flat normal texture (0.5, 0.5, 1.0 — pointing up).
     */
    TextureHandle GetDefaultNormal();

    /**
     * Get a default 1x1 black texture (for missing AO/emissive maps).
     */
    TextureHandle GetDefaultBlack();

private:
    void CreateDefaultTextures();

    filament::Engine* engine_ = nullptr;

    // All textures owned by this system
    std::vector<filament::Texture*> textures_;

    // Cache: canonical file path → TextureHandle
    std::unordered_map<std::string, TextureHandle> cache_;

    // Default textures
    TextureHandle default_white_;
    TextureHandle default_normal_;
    TextureHandle default_black_;
};

} // namespace se
