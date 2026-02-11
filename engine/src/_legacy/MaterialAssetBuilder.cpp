#include "engine/assets/MaterialAssetBuilder.h"

#include <random>
#include <sstream>
#include <iomanip>

#include <stb_image.h>

#include "engine/assets/RawTextureCompressor.h"
#include "engine/Log.h"

namespace se {

MaterialAssetBuilder::MaterialAssetBuilder()
    : compressor_(std::make_shared<RawTextureCompressor>()) {
    asset_.uuid = GenerateUUID();
}

MaterialAssetBuilder::MaterialAssetBuilder(std::shared_ptr<ITextureCompressor> compressor)
    : compressor_(compressor ? std::move(compressor) : std::make_shared<RawTextureCompressor>()) {
    asset_.uuid = GenerateUUID();
}

MaterialAssetBuilder& MaterialAssetBuilder::WithName(const std::string& name) {
    asset_.name = name;
    asset_.definition.name = name;
    return *this;
}

MaterialAssetBuilder& MaterialAssetBuilder::WithUUID(const std::string& uuid) {
    asset_.uuid = uuid;
    asset_.definition.uuid = uuid;
    return *this;
}

MaterialAssetBuilder& MaterialAssetBuilder::WithBaseColor(const Vector4& color) {
    asset_.definition.baseColor = color;
    return *this;
}

MaterialAssetBuilder& MaterialAssetBuilder::WithMetallic(float value) {
    asset_.definition.metallic = value;
    return *this;
}

MaterialAssetBuilder& MaterialAssetBuilder::WithRoughness(float value) {
    asset_.definition.roughness = value;
    return *this;
}

MaterialAssetBuilder& MaterialAssetBuilder::WithReflectance(float value) {
    asset_.definition.reflectance = value;
    return *this;
}

MaterialAssetBuilder& MaterialAssetBuilder::WithAO(float value) {
    asset_.definition.ao = value;
    return *this;
}

MaterialAssetBuilder& MaterialAssetBuilder::WithNormalScale(float value) {
    asset_.definition.normalScale = value;
    return *this;
}

MaterialAssetBuilder& MaterialAssetBuilder::WithEmissive(const Vector3& color, float factor) {
    asset_.definition.emissiveColor = color;
    asset_.definition.emissiveFactor = factor;
    if (factor > 0.0f) {
        asset_.definition.features |= MATERIAL_FEATURE_EMISSIVE;
    }
    return *this;
}

MaterialAssetBuilder& MaterialAssetBuilder::WithClearCoat(float intensity, float roughness) {
    asset_.definition.clearCoat = intensity;
    asset_.definition.clearCoatRoughness = roughness;
    if (intensity > 0.0f) {
        asset_.definition.features |= MATERIAL_FEATURE_CLEAR_COAT;
    }
    return *this;
}

MaterialAssetBuilder& MaterialAssetBuilder::WithAnisotropy(float value, const Vector3& direction) {
    asset_.definition.anisotropy = value;
    asset_.definition.anisotropyDirection = direction;
    if (std::abs(value) > 0.01f) {
        asset_.definition.features |= MATERIAL_FEATURE_ANISOTROPY;
    }
    return *this;
}

MaterialAssetBuilder& MaterialAssetBuilder::WithSubsurface(const Vector3& color, float power, float thickness) {
    asset_.definition.subsurfaceColor = color;
    asset_.definition.subsurfacePower = power;
    asset_.definition.thickness = thickness;
    if (power > 0.0f) {
        asset_.definition.features |= MATERIAL_FEATURE_SUBSURFACE;
    }
    return *this;
}

MaterialAssetBuilder& MaterialAssetBuilder::WithTransmission(float value, float ior) {
    asset_.definition.transmission = value;
    asset_.definition.ior = ior;
    if (value > 0.0f) {
        asset_.definition.features |= MATERIAL_FEATURE_TRANSMISSION;
    }
    return *this;
}

MaterialAssetBuilder& MaterialAssetBuilder::WithAlbedoTexture(const std::string& path) {
    asset_.albedoSourcePath = path;
    asset_.definition.albedoPath = path;
    auto data = LoadAndCompressTexture(path);
    if (data.IsValid()) {
        asset_.albedoTexture = std::move(data);
        asset_.definition.features |= MATERIAL_FEATURE_ALBEDO_MAP;
    }
    return *this;
}

MaterialAssetBuilder& MaterialAssetBuilder::WithNormalTexture(const std::string& path) {
    asset_.normalSourcePath = path;
    asset_.definition.normalPath = path;
    auto data = LoadAndCompressTexture(path);
    if (data.IsValid()) {
        asset_.normalTexture = std::move(data);
        asset_.definition.features |= MATERIAL_FEATURE_NORMAL_MAP;
    }
    return *this;
}

MaterialAssetBuilder& MaterialAssetBuilder::WithMetallicTexture(const std::string& path) {
    asset_.metallicSourcePath = path;
    asset_.definition.metallicPath = path;
    auto data = LoadAndCompressTexture(path);
    if (data.IsValid()) {
        asset_.metallicTexture = std::move(data);
        asset_.definition.features |= MATERIAL_FEATURE_METALLIC_MAP;
    }
    return *this;
}

MaterialAssetBuilder& MaterialAssetBuilder::WithRoughnessTexture(const std::string& path) {
    asset_.roughnessSourcePath = path;
    asset_.definition.roughnessPath = path;
    auto data = LoadAndCompressTexture(path);
    if (data.IsValid()) {
        asset_.roughnessTexture = std::move(data);
        asset_.definition.features |= MATERIAL_FEATURE_ROUGHNESS_MAP;
    }
    return *this;
}

MaterialAssetBuilder& MaterialAssetBuilder::WithAOTexture(const std::string& path) {
    asset_.aoSourcePath = path;
    asset_.definition.aoPath = path;
    auto data = LoadAndCompressTexture(path);
    if (data.IsValid()) {
        asset_.aoTexture = std::move(data);
        asset_.definition.features |= MATERIAL_FEATURE_AO_MAP;
    }
    return *this;
}

MaterialAssetBuilder& MaterialAssetBuilder::WithEmissiveTexture(const std::string& path) {
    asset_.emissiveSourcePath = path;
    asset_.definition.emissivePath = path;
    auto data = LoadAndCompressTexture(path);
    if (data.IsValid()) {
        asset_.emissiveTexture = std::move(data);
        asset_.definition.features |= MATERIAL_FEATURE_EMISSIVE_MAP;
    }
    return *this;
}

MaterialAssetBuilder& MaterialAssetBuilder::WithAlbedoData(const EmbeddedTextureData& data) {
    asset_.albedoTexture = data;
    if (data.IsValid()) {
        asset_.definition.features |= MATERIAL_FEATURE_ALBEDO_MAP;
    }
    return *this;
}

MaterialAssetBuilder& MaterialAssetBuilder::WithNormalData(const EmbeddedTextureData& data) {
    asset_.normalTexture = data;
    if (data.IsValid()) {
        asset_.definition.features |= MATERIAL_FEATURE_NORMAL_MAP;
    }
    return *this;
}

MaterialAssetBuilder& MaterialAssetBuilder::WithMetallicData(const EmbeddedTextureData& data) {
    asset_.metallicTexture = data;
    if (data.IsValid()) {
        asset_.definition.features |= MATERIAL_FEATURE_METALLIC_MAP;
    }
    return *this;
}

MaterialAssetBuilder& MaterialAssetBuilder::WithRoughnessData(const EmbeddedTextureData& data) {
    asset_.roughnessTexture = data;
    if (data.IsValid()) {
        asset_.definition.features |= MATERIAL_FEATURE_ROUGHNESS_MAP;
    }
    return *this;
}

MaterialAssetBuilder& MaterialAssetBuilder::WithAOData(const EmbeddedTextureData& data) {
    asset_.aoTexture = data;
    if (data.IsValid()) {
        asset_.definition.features |= MATERIAL_FEATURE_AO_MAP;
    }
    return *this;
}

MaterialAssetBuilder& MaterialAssetBuilder::WithEmissiveData(const EmbeddedTextureData& data) {
    asset_.emissiveTexture = data;
    if (data.IsValid()) {
        asset_.definition.features |= MATERIAL_FEATURE_EMISSIVE_MAP;
    }
    return *this;
}

MaterialAsset MaterialAssetBuilder::Build() {
    asset_.definition.UpdateFeaturesFromPaths();
    SE_LOG_INFO("MaterialAssetBuilder: Built material '{}' (features: 0x{:X})",
                asset_.name, asset_.definition.features);
    return std::move(asset_);
}

std::string MaterialAssetBuilder::GenerateUUID() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 15);
    static std::uniform_int_distribution<> dis2(8, 11);
    
    std::stringstream ss;
    ss << std::hex;
    
    for (int i = 0; i < 8; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 4; i++) ss << dis(gen);
    ss << "-4";
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    ss << dis2(gen);
    for (int i = 0; i < 3; i++) ss << dis(gen);
    ss << "-";
    for (int i = 0; i < 12; i++) ss << dis(gen);
    
    return ss.str();
}

EmbeddedTextureData MaterialAssetBuilder::LoadAndCompressTexture(const std::string& path) {
    EmbeddedTextureData result;
    
    if (path.empty()) {
        SE_LOG_WARN("MaterialAssetBuilder: Empty texture path");
        return result;
    }
    
    SE_LOG_INFO("MaterialAssetBuilder: Loading texture from '{}'", path);
    
    stbi_set_flip_vertically_on_load(true);
    
    int width, height, channels;
    unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
    
    if (!data) {
        SE_LOG_ERROR("MaterialAssetBuilder: Failed to load texture '{}': {}",
                     path, stbi_failure_reason());
        return result;
    }
    
    result.header.width = static_cast<uint32_t>(width);
    result.header.height = static_cast<uint32_t>(height);
    result.header.channels = static_cast<uint8_t>(channels);
    result.header.mipLevels = 1;
    
    TextureCompressionFormat format;
    switch (channels) {
        case 4: format = TextureCompressionFormat::Raw_RGBA8; break;
        case 3: format = TextureCompressionFormat::Raw_RGB8; break;
        case 2: format = TextureCompressionFormat::Raw_RG8; break;
        case 1: format = TextureCompressionFormat::Raw_R8; break;
        default: format = TextureCompressionFormat::Raw_RGBA8; break;
    }
    result.header.SetFormat(format);
    
    size_t dataSize = CalculateRawTextureSize(width, height, channels);
    result.pixels = compressor_->Compress(data, width, height, channels);
    result.header.dataSize = static_cast<uint32_t>(result.pixels.size());
    
    stbi_image_free(data);
    
    SE_LOG_INFO("MaterialAssetBuilder: Texture loaded ({}x{}, {} channels, {} bytes)",
                width, height, channels, result.header.dataSize);
    
    return result;
}

}  // namespace se
