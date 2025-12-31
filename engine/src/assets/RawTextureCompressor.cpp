#include "engine/assets/RawTextureCompressor.h"

namespace se {

TextureCompressionFormat RawTextureCompressor::GetFormat() const {
    return TextureCompressionFormat::Raw_RGBA8;
}

std::vector<uint8_t> RawTextureCompressor::Compress(
    const uint8_t* rawData,
    uint32_t width,
    uint32_t height,
    uint8_t channels
) const {
    if (!rawData || width == 0 || height == 0 || channels == 0) {
        return {};
    }
    
    size_t dataSize = CalculateRawTextureSize(width, height, channels);
    std::vector<uint8_t> result(dataSize);
    std::memcpy(result.data(), rawData, dataSize);
    return result;
}

std::vector<uint8_t> RawTextureCompressor::Decompress(
    const uint8_t* compressedData,
    uint32_t dataSize,
    uint32_t width,
    uint32_t height,
    uint8_t channels
) const {
    if (!compressedData || dataSize == 0) {
        return {};
    }
    
    std::vector<uint8_t> result(dataSize);
    std::memcpy(result.data(), compressedData, dataSize);
    return result;
}

size_t RawTextureCompressor::EstimateCompressedSize(
    uint32_t width,
    uint32_t height,
    uint8_t channels
) const {
    return CalculateRawTextureSize(width, height, channels);
}

}  // namespace se
