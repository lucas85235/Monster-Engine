#pragma once
/**
 * MaterialAssetTypes.h - Fundamental types for the Material Asset system.
 *
 * Defines enums, headers, and base structures for texture compression
 * and embedded texture data used in binary material serialization.
 */

#include <cstdint>
#include <vector>

namespace se {

enum class TextureCompressionFormat : uint8_t {
    Raw_RGBA8 = 0,  // No compression, 32bpp (4 bytes per pixel)
    Raw_RGB8  = 1,  // No compression, 24bpp (3 bytes per pixel)
    Raw_RG8   = 2,  // No compression, 16bpp (2 bytes per pixel)
    Raw_R8    = 3,  // No compression, 8bpp (1 byte per pixel)
    DXT1      = 10, // BC1 - No alpha, 4bpp (future)
    DXT5      = 11, // BC3 - With alpha, 8bpp (future)
    BC7       = 12, // High quality, 8bpp (future)
};

#pragma pack(push, 1)
struct EmbeddedTextureHeader {
    uint32_t width      = 0;
    uint32_t height     = 0;
    uint8_t  channels   = 4;
    uint8_t  format     = static_cast<uint8_t>(TextureCompressionFormat::Raw_RGBA8);
    uint8_t  mipLevels  = 1;
    uint8_t  reserved   = 0;
    uint32_t dataSize   = 0;
    
    TextureCompressionFormat GetFormat() const {
        return static_cast<TextureCompressionFormat>(format);
    }
    
    void SetFormat(TextureCompressionFormat fmt) {
        format = static_cast<uint8_t>(fmt);
    }
};
#pragma pack(pop)

static_assert(sizeof(EmbeddedTextureHeader) == 16, "EmbeddedTextureHeader must be 16 bytes");

struct EmbeddedTextureData {
    EmbeddedTextureHeader header;
    std::vector<uint8_t> pixels;
    
    bool IsValid() const {
        return header.width > 0 && header.height > 0 && !pixels.empty();
    }
    
    size_t GetExpectedSize() const {
        return static_cast<size_t>(header.width) * header.height * header.channels;
    }
    
    void Clear() {
        header = {};
        pixels.clear();
    }
};

constexpr uint8_t GetChannelsForFormat(TextureCompressionFormat format) {
    switch (format) {
        case TextureCompressionFormat::Raw_RGBA8: return 4;
        case TextureCompressionFormat::Raw_RGB8:  return 3;
        case TextureCompressionFormat::Raw_RG8:   return 2;
        case TextureCompressionFormat::Raw_R8:    return 1;
        default: return 4;
    }
}

constexpr size_t CalculateRawTextureSize(uint32_t width, uint32_t height, uint8_t channels) {
    return static_cast<size_t>(width) * height * channels;
}

}  // namespace se
