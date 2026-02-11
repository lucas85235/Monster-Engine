#pragma once
/**
 * MaterialAssetSerializer.h - High-performance binary material serializer.
 *
 * Implements IMaterialAssetSerializer with support for embedded textures
 * and configurable compression strategies.
 */

#include <memory>

#include "engine/assets/IMaterialAssetSerializer.h"
#include "engine/assets/ITextureCompressor.h"
#include "engine/assets/MaterialAsset.h"

namespace se {

class MaterialAssetSerializer : public IMaterialAssetSerializer {
public:
    MaterialAssetSerializer();
    explicit MaterialAssetSerializer(std::shared_ptr<ITextureCompressor> compressor);
    
    bool Serialize(const MaterialAsset& asset, std::ostream& stream) const override;
    bool Deserialize(MaterialAsset& asset, std::istream& stream) const override;
    uint32_t GetVersion() const override { return MaterialAsset::VERSION; }
    
    MaterialAssetSerializer& WithCompressor(std::shared_ptr<ITextureCompressor> compressor);
    MaterialAssetSerializer& EmbedTextures(bool embed);
    
    bool ShouldEmbedTextures() const { return embedTextures_; }
    
private:
    std::shared_ptr<ITextureCompressor> compressor_;
    bool embedTextures_ = true;
    
    void WriteString(std::ostream& stream, const std::string& str) const;
    bool ReadString(std::istream& stream, std::string& str) const;
    
    void WriteTextureData(std::ostream& stream, const EmbeddedTextureData& tex) const;
    bool ReadTextureData(std::istream& stream, EmbeddedTextureData& tex) const;
    
    void WriteMaterialDefinition(std::ostream& stream, const MaterialDefinition& def) const;
    bool ReadMaterialDefinition(std::istream& stream, MaterialDefinition& def) const;
};

}  // namespace se
