#pragma once

#include <cstdint>
#include <limits>

namespace filament {
class Texture;
} // namespace filament

namespace se {

class TextureSystem;

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
    bool IsValid() const;
    explicit operator bool() const { return IsValid(); }

    /** Get the texture dimensions. */
    uint32_t GetWidth() const;
    uint32_t GetHeight() const;

    /** Direct access for advanced use (e.g., setting on MaterialInstance). */
    filament::Texture* GetNative() const;

private:
    friend class TextureSystem;
    static constexpr uint32_t kInvalidIndex = std::numeric_limits<uint32_t>::max();

    TextureHandle(TextureSystem* owner, uint32_t index, uint32_t generation)
        : owner_(owner), index_(index), generation_(generation) {}

    TextureSystem* owner_      = nullptr;
    uint32_t       index_      = kInvalidIndex;
    uint32_t       generation_ = 0;
};

} // namespace se
