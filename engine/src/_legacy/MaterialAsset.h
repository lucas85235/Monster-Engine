#pragma once
/**
 * MaterialAsset.h - Main material asset structure.
 *
 * Contains all material data including PBR parameters and embedded textures.
 * Designed for high-performance binary serialization.
 */

#include <optional>
#include <string>

#include "engine/assets/MaterialAssetTypes.h"
#include "engine/renderer/MaterialDefinition.h"

namespace se {

struct MaterialAsset {
    static constexpr uint32_t MAGIC   = 0x4D415432;  // "MAT2"
    static constexpr uint32_t VERSION = 2;
    
    std::string uuid;
    std::string name = "Default Material";
    
    MaterialDefinition definition;
    
    std::optional<EmbeddedTextureData> albedoTexture;
    std::optional<EmbeddedTextureData> normalTexture;
    std::optional<EmbeddedTextureData> metallicTexture;
    std::optional<EmbeddedTextureData> roughnessTexture;
    std::optional<EmbeddedTextureData> aoTexture;
    std::optional<EmbeddedTextureData> emissiveTexture;
    
    std::string albedoSourcePath;
    std::string normalSourcePath;
    std::string metallicSourcePath;
    std::string roughnessSourcePath;
    std::string aoSourcePath;
    std::string emissiveSourcePath;
    
    bool HasEmbeddedTextures() const {
        return albedoTexture.has_value()   ||
               normalTexture.has_value()   ||
               metallicTexture.has_value() ||
               roughnessTexture.has_value()||
               aoTexture.has_value()       ||
               emissiveTexture.has_value();
    }
    
    size_t GetTotalTextureDataSize() const {
        size_t total = 0;
        if (albedoTexture)    total += albedoTexture->pixels.size();
        if (normalTexture)    total += normalTexture->pixels.size();
        if (metallicTexture)  total += metallicTexture->pixels.size();
        if (roughnessTexture) total += roughnessTexture->pixels.size();
        if (aoTexture)        total += aoTexture->pixels.size();
        if (emissiveTexture)  total += emissiveTexture->pixels.size();
        return total;
    }
    
    uint8_t GetTextureFlags() const {
        uint8_t flags = 0;
        if (albedoTexture.has_value()   && albedoTexture->IsValid())   flags |= (1 << 0);
        if (normalTexture.has_value()   && normalTexture->IsValid())   flags |= (1 << 1);
        if (metallicTexture.has_value() && metallicTexture->IsValid()) flags |= (1 << 2);
        if (roughnessTexture.has_value()&& roughnessTexture->IsValid())flags |= (1 << 3);
        if (aoTexture.has_value()       && aoTexture->IsValid())       flags |= (1 << 4);
        if (emissiveTexture.has_value() && emissiveTexture->IsValid()) flags |= (1 << 5);
        return flags;
    }
    
    void ClearEmbeddedTextures() {
        albedoTexture.reset();
        normalTexture.reset();
        metallicTexture.reset();
        roughnessTexture.reset();
        aoTexture.reset();
        emissiveTexture.reset();
    }
    
    static MaterialAsset CreateDefault() {
        MaterialAsset asset;
        asset.name = "Default";
        asset.definition = MaterialDefinition::CreateDefault();
        return asset;
    }
};

}  // namespace se
