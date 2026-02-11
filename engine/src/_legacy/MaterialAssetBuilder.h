#pragma once
/**
 * MaterialAssetBuilder.h - Builder pattern for MaterialAsset construction.
 *
 * Provides a fluent interface for building MaterialAsset objects with
 * automatic texture loading and embedding.
 */

#include <memory>
#include <string>

#include "engine/assets/MaterialAsset.h"
#include "engine/assets/ITextureCompressor.h"

namespace se {

class MaterialAssetBuilder {
public:
    MaterialAssetBuilder();
    explicit MaterialAssetBuilder(std::shared_ptr<ITextureCompressor> compressor);
    
    MaterialAssetBuilder& WithName(const std::string& name);
    MaterialAssetBuilder& WithUUID(const std::string& uuid);
    
    MaterialAssetBuilder& WithBaseColor(const Vector4& color);
    MaterialAssetBuilder& WithMetallic(float value);
    MaterialAssetBuilder& WithRoughness(float value);
    MaterialAssetBuilder& WithReflectance(float value);
    MaterialAssetBuilder& WithAO(float value);
    MaterialAssetBuilder& WithNormalScale(float value);
    MaterialAssetBuilder& WithEmissive(const Vector3& color, float factor);
    
    MaterialAssetBuilder& WithClearCoat(float intensity, float roughness);
    MaterialAssetBuilder& WithAnisotropy(float value, const Vector3& direction);
    MaterialAssetBuilder& WithSubsurface(const Vector3& color, float power, float thickness);
    MaterialAssetBuilder& WithTransmission(float value, float ior);
    
    MaterialAssetBuilder& WithAlbedoTexture(const std::string& path);
    MaterialAssetBuilder& WithNormalTexture(const std::string& path);
    MaterialAssetBuilder& WithMetallicTexture(const std::string& path);
    MaterialAssetBuilder& WithRoughnessTexture(const std::string& path);
    MaterialAssetBuilder& WithAOTexture(const std::string& path);
    MaterialAssetBuilder& WithEmissiveTexture(const std::string& path);
    
    MaterialAssetBuilder& WithAlbedoData(const EmbeddedTextureData& data);
    MaterialAssetBuilder& WithNormalData(const EmbeddedTextureData& data);
    MaterialAssetBuilder& WithMetallicData(const EmbeddedTextureData& data);
    MaterialAssetBuilder& WithRoughnessData(const EmbeddedTextureData& data);
    MaterialAssetBuilder& WithAOData(const EmbeddedTextureData& data);
    MaterialAssetBuilder& WithEmissiveData(const EmbeddedTextureData& data);
    
    MaterialAsset Build();
    
    static std::string GenerateUUID();
    
private:
    MaterialAsset asset_;
    std::shared_ptr<ITextureCompressor> compressor_;
    
    EmbeddedTextureData LoadAndCompressTexture(const std::string& path);
};

}  // namespace se
