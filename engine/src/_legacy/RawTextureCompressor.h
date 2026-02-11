#pragma once
/**
 * RawTextureCompressor.h - No-compression texture compressor.
 *
 * Implements ITextureCompressor with zero-overhead compression.
 * Simply copies raw pixel data for maximum deserialization performance.
 */

#include "engine/assets/ITextureCompressor.h"

namespace se {

class RawTextureCompressor : public ITextureCompressor {
public:
    TextureCompressionFormat GetFormat() const override;
    
    std::vector<uint8_t> Compress(
        const uint8_t* rawData,
        uint32_t width,
        uint32_t height,
        uint8_t channels
    ) const override;
    
    std::vector<uint8_t> Decompress(
        const uint8_t* compressedData,
        uint32_t dataSize,
        uint32_t width,
        uint32_t height,
        uint8_t channels
    ) const override;
    
    size_t EstimateCompressedSize(
        uint32_t width,
        uint32_t height,
        uint8_t channels
    ) const override;
};

}  // namespace se
