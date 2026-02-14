#pragma once

#include <cstdint>

namespace filament {
class Texture;
} // namespace filament

namespace se {

/**
 * Opaque handle to a Filament Texture.
 *
 * Engine/game code uses this instead of touching filament::Texture directly.
 * Lifetime: owned by TextureSystem. Handle becomes invalid if the
 * TextureSystem destroys the underlying texture.
 */
class TextureHandle {
public:
    TextureHandle() = default;

    /** Check if handle points to a valid texture. */
    bool IsValid() const { return texture_ != nullptr; }
    explicit operator bool() const { return IsValid(); }

    /** Get the texture dimensions. */
    uint32_t GetWidth() const { return width_; }
    uint32_t GetHeight() const { return height_; }

    /** Direct access for advanced use (e.g., setting on MaterialInstance). */
    filament::Texture* GetNative() const { return texture_; }

private:
    friend class TextureSystem;
    TextureHandle(filament::Texture* texture, uint32_t width, uint32_t height)
        : texture_(texture), width_(width), height_(height) {}

    filament::Texture* texture_ = nullptr;
    uint32_t width_  = 0;
    uint32_t height_ = 0;
};

} // namespace se
