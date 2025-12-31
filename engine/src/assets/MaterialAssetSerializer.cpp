#include "engine/assets/MaterialAssetSerializer.h"

#include "engine/assets/RawTextureCompressor.h"
#include "engine/Log.h"

namespace se {

MaterialAssetSerializer::MaterialAssetSerializer()
    : compressor_(std::make_shared<RawTextureCompressor>()) {
}

MaterialAssetSerializer::MaterialAssetSerializer(std::shared_ptr<ITextureCompressor> compressor)
    : compressor_(compressor ? std::move(compressor) : std::make_shared<RawTextureCompressor>()) {
}

MaterialAssetSerializer& MaterialAssetSerializer::WithCompressor(std::shared_ptr<ITextureCompressor> compressor) {
    compressor_ = compressor ? std::move(compressor) : std::make_shared<RawTextureCompressor>();
    return *this;
}

MaterialAssetSerializer& MaterialAssetSerializer::EmbedTextures(bool embed) {
    embedTextures_ = embed;
    return *this;
}

bool MaterialAssetSerializer::Serialize(const MaterialAsset& asset, std::ostream& stream) const {
    if (!stream.good()) {
        SE_LOG_ERROR("MaterialAssetSerializer: Invalid output stream");
        return false;
    }
    
    SE_LOG_INFO("MaterialAssetSerializer: Serializing material '{}' (embedded textures: {})",
                asset.name, embedTextures_);
    
    uint32_t magic = MaterialAsset::MAGIC;
    uint32_t version = MaterialAsset::VERSION;
    stream.write(reinterpret_cast<const char*>(&magic), sizeof(magic));
    stream.write(reinterpret_cast<const char*>(&version), sizeof(version));
    
    WriteString(stream, asset.uuid);
    WriteString(stream, asset.name);
    
    WriteMaterialDefinition(stream, asset.definition);
    
    WriteString(stream, asset.albedoSourcePath);
    WriteString(stream, asset.normalSourcePath);
    WriteString(stream, asset.metallicSourcePath);
    WriteString(stream, asset.roughnessSourcePath);
    WriteString(stream, asset.aoSourcePath);
    WriteString(stream, asset.emissiveSourcePath);
    
    uint8_t textureFlags = 0;
    if (embedTextures_) {
        textureFlags = asset.GetTextureFlags();
    }
    stream.write(reinterpret_cast<const char*>(&textureFlags), sizeof(textureFlags));
    
    if (textureFlags & (1 << 0) && asset.albedoTexture) {
        WriteTextureData(stream, *asset.albedoTexture);
    }
    if (textureFlags & (1 << 1) && asset.normalTexture) {
        WriteTextureData(stream, *asset.normalTexture);
    }
    if (textureFlags & (1 << 2) && asset.metallicTexture) {
        WriteTextureData(stream, *asset.metallicTexture);
    }
    if (textureFlags & (1 << 3) && asset.roughnessTexture) {
        WriteTextureData(stream, *asset.roughnessTexture);
    }
    if (textureFlags & (1 << 4) && asset.aoTexture) {
        WriteTextureData(stream, *asset.aoTexture);
    }
    if (textureFlags & (1 << 5) && asset.emissiveTexture) {
        WriteTextureData(stream, *asset.emissiveTexture);
    }
    
    SE_LOG_INFO("MaterialAssetSerializer: Material serialized successfully (texture flags: 0x{:02X})",
                textureFlags);
    return stream.good();
}

bool MaterialAssetSerializer::Deserialize(MaterialAsset& asset, std::istream& stream) const {
    if (!stream.good()) {
        SE_LOG_ERROR("MaterialAssetSerializer: Invalid input stream");
        return false;
    }
    
    uint32_t magic = 0;
    uint32_t version = 0;
    stream.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    stream.read(reinterpret_cast<char*>(&version), sizeof(version));
    
    if (magic != MaterialAsset::MAGIC) {
        SE_LOG_ERROR("MaterialAssetSerializer: Invalid file magic (expected 0x{:08X}, got 0x{:08X})",
                     MaterialAsset::MAGIC, magic);
        return false;
    }
    
    if (version > MaterialAsset::VERSION) {
        SE_LOG_ERROR("MaterialAssetSerializer: Unsupported version {} (max: {})",
                     version, MaterialAsset::VERSION);
        return false;
    }
    
    if (!ReadString(stream, asset.uuid)) return false;
    if (!ReadString(stream, asset.name)) return false;
    
    SE_LOG_INFO("MaterialAssetSerializer: Deserializing material '{}' (version {})",
                asset.name, version);
    
    if (!ReadMaterialDefinition(stream, asset.definition)) return false;
    
    if (!ReadString(stream, asset.albedoSourcePath)) return false;
    if (!ReadString(stream, asset.normalSourcePath)) return false;
    if (!ReadString(stream, asset.metallicSourcePath)) return false;
    if (!ReadString(stream, asset.roughnessSourcePath)) return false;
    if (!ReadString(stream, asset.aoSourcePath)) return false;
    if (!ReadString(stream, asset.emissiveSourcePath)) return false;
    
    uint8_t textureFlags = 0;
    stream.read(reinterpret_cast<char*>(&textureFlags), sizeof(textureFlags));
    
    asset.ClearEmbeddedTextures();
    
    if (textureFlags & (1 << 0)) {
        asset.albedoTexture = EmbeddedTextureData{};
        if (!ReadTextureData(stream, *asset.albedoTexture)) return false;
    }
    if (textureFlags & (1 << 1)) {
        asset.normalTexture = EmbeddedTextureData{};
        if (!ReadTextureData(stream, *asset.normalTexture)) return false;
    }
    if (textureFlags & (1 << 2)) {
        asset.metallicTexture = EmbeddedTextureData{};
        if (!ReadTextureData(stream, *asset.metallicTexture)) return false;
    }
    if (textureFlags & (1 << 3)) {
        asset.roughnessTexture = EmbeddedTextureData{};
        if (!ReadTextureData(stream, *asset.roughnessTexture)) return false;
    }
    if (textureFlags & (1 << 4)) {
        asset.aoTexture = EmbeddedTextureData{};
        if (!ReadTextureData(stream, *asset.aoTexture)) return false;
    }
    if (textureFlags & (1 << 5)) {
        asset.emissiveTexture = EmbeddedTextureData{};
        if (!ReadTextureData(stream, *asset.emissiveTexture)) return false;
    }
    
    SE_LOG_INFO("MaterialAssetSerializer: Material deserialized (texture flags: 0x{:02X})",
                textureFlags);
    return stream.good();
}

void MaterialAssetSerializer::WriteString(std::ostream& stream, const std::string& str) const {
    uint32_t length = static_cast<uint32_t>(str.size());
    stream.write(reinterpret_cast<const char*>(&length), sizeof(length));
    if (length > 0) {
        stream.write(str.data(), length);
    }
}

bool MaterialAssetSerializer::ReadString(std::istream& stream, std::string& str) const {
    uint32_t length = 0;
    stream.read(reinterpret_cast<char*>(&length), sizeof(length));
    if (stream.fail()) return false;
    
    if (length > 0) {
        str.resize(length);
        stream.read(str.data(), length);
        if (stream.fail()) return false;
    } else {
        str.clear();
    }
    return true;
}

void MaterialAssetSerializer::WriteTextureData(std::ostream& stream, const EmbeddedTextureData& tex) const {
    stream.write(reinterpret_cast<const char*>(&tex.header), sizeof(tex.header));
    if (tex.header.dataSize > 0 && !tex.pixels.empty()) {
        stream.write(reinterpret_cast<const char*>(tex.pixels.data()), tex.header.dataSize);
    }
}

bool MaterialAssetSerializer::ReadTextureData(std::istream& stream, EmbeddedTextureData& tex) const {
    stream.read(reinterpret_cast<char*>(&tex.header), sizeof(tex.header));
    if (stream.fail()) return false;
    
    if (tex.header.dataSize > 0) {
        tex.pixels.resize(tex.header.dataSize);
        stream.read(reinterpret_cast<char*>(tex.pixels.data()), tex.header.dataSize);
        if (stream.fail()) return false;
    }
    return true;
}

void MaterialAssetSerializer::WriteMaterialDefinition(std::ostream& stream, const MaterialDefinition& def) const {
    stream.write(reinterpret_cast<const char*>(&def.baseColor), sizeof(def.baseColor));
    stream.write(reinterpret_cast<const char*>(&def.emissiveColor), sizeof(def.emissiveColor));
    stream.write(reinterpret_cast<const char*>(&def.emissiveFactor), sizeof(def.emissiveFactor));
    stream.write(reinterpret_cast<const char*>(&def.metallic), sizeof(def.metallic));
    stream.write(reinterpret_cast<const char*>(&def.roughness), sizeof(def.roughness));
    stream.write(reinterpret_cast<const char*>(&def.reflectance), sizeof(def.reflectance));
    stream.write(reinterpret_cast<const char*>(&def.ao), sizeof(def.ao));
    stream.write(reinterpret_cast<const char*>(&def.normalScale), sizeof(def.normalScale));
    stream.write(reinterpret_cast<const char*>(&def.clearCoat), sizeof(def.clearCoat));
    stream.write(reinterpret_cast<const char*>(&def.clearCoatRoughness), sizeof(def.clearCoatRoughness));
    stream.write(reinterpret_cast<const char*>(&def.anisotropy), sizeof(def.anisotropy));
    stream.write(reinterpret_cast<const char*>(&def.anisotropyDirection), sizeof(def.anisotropyDirection));
    stream.write(reinterpret_cast<const char*>(&def.sheenColor), sizeof(def.sheenColor));
    stream.write(reinterpret_cast<const char*>(&def.sheenRoughness), sizeof(def.sheenRoughness));
    stream.write(reinterpret_cast<const char*>(&def.subsurfaceColor), sizeof(def.subsurfaceColor));
    stream.write(reinterpret_cast<const char*>(&def.subsurfacePower), sizeof(def.subsurfacePower));
    stream.write(reinterpret_cast<const char*>(&def.thickness), sizeof(def.thickness));
    stream.write(reinterpret_cast<const char*>(&def.transmission), sizeof(def.transmission));
    stream.write(reinterpret_cast<const char*>(&def.ior), sizeof(def.ior));
    stream.write(reinterpret_cast<const char*>(&def.features), sizeof(def.features));
}

bool MaterialAssetSerializer::ReadMaterialDefinition(std::istream& stream, MaterialDefinition& def) const {
    stream.read(reinterpret_cast<char*>(&def.baseColor), sizeof(def.baseColor));
    stream.read(reinterpret_cast<char*>(&def.emissiveColor), sizeof(def.emissiveColor));
    stream.read(reinterpret_cast<char*>(&def.emissiveFactor), sizeof(def.emissiveFactor));
    stream.read(reinterpret_cast<char*>(&def.metallic), sizeof(def.metallic));
    stream.read(reinterpret_cast<char*>(&def.roughness), sizeof(def.roughness));
    stream.read(reinterpret_cast<char*>(&def.reflectance), sizeof(def.reflectance));
    stream.read(reinterpret_cast<char*>(&def.ao), sizeof(def.ao));
    stream.read(reinterpret_cast<char*>(&def.normalScale), sizeof(def.normalScale));
    stream.read(reinterpret_cast<char*>(&def.clearCoat), sizeof(def.clearCoat));
    stream.read(reinterpret_cast<char*>(&def.clearCoatRoughness), sizeof(def.clearCoatRoughness));
    stream.read(reinterpret_cast<char*>(&def.anisotropy), sizeof(def.anisotropy));
    stream.read(reinterpret_cast<char*>(&def.anisotropyDirection), sizeof(def.anisotropyDirection));
    stream.read(reinterpret_cast<char*>(&def.sheenColor), sizeof(def.sheenColor));
    stream.read(reinterpret_cast<char*>(&def.sheenRoughness), sizeof(def.sheenRoughness));
    stream.read(reinterpret_cast<char*>(&def.subsurfaceColor), sizeof(def.subsurfaceColor));
    stream.read(reinterpret_cast<char*>(&def.subsurfacePower), sizeof(def.subsurfacePower));
    stream.read(reinterpret_cast<char*>(&def.thickness), sizeof(def.thickness));
    stream.read(reinterpret_cast<char*>(&def.transmission), sizeof(def.transmission));
    stream.read(reinterpret_cast<char*>(&def.ior), sizeof(def.ior));
    stream.read(reinterpret_cast<char*>(&def.features), sizeof(def.features));
    return !stream.fail();
}

}  // namespace se
