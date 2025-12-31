#pragma once
/**
 * ITextureCompressor.h - Strategy interface for texture compression.
 *
 * Enables different compression strategies (Raw, DXT, BC7) to be used
 * interchangeably when serializing material assets.
 */

#include <cstdint>
#include <vector>

#include "engine/assets/MaterialAssetTypes.h"

namespace se {

class ITextureCompressor {
public:
    virtual ~ITextureCompressor() = default;
    
    virtual TextureCompressionFormat GetFormat() const = 0;
    
    virtual std::vector<uint8_t> Compress(
        const uint8_t* rawData,
        uint32_t width,
        uint32_t height,
        uint8_t channels
    ) const = 0;
    
    virtual std::vector<uint8_t> Decompress(
        const uint8_t* compressedData,
        uint32_t dataSize,
        uint32_t width,
        uint32_t height,
        uint8_t channels
    ) const = 0;
    
    virtual size_t EstimateCompressedSize(
        uint32_t width,
        uint32_t height,
        uint8_t channels
    ) const = 0;
};

}  // namespace se
