#include "core/MaterialSerializer.h"

#include <fstream>
#include <sstream>

#include "engine/assets/MaterialAssetSerializer.h"
#include "engine/Log.h"

namespace mst {

bool MaterialSerializer::Save(const EditorMaterialData& data, const std::filesystem::path& path) {
    SE_LOG_INFO("MaterialSerializer: Saving material '{}' to {}", data.name, path.string());
    
    se::MaterialAsset asset = data.ToEngineAsset();
    
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        SE_LOG_ERROR("MaterialSerializer: Failed to open file for writing: {}", path.string());
        return false;
    }
    
    se::MaterialAssetSerializer serializer;
    serializer.EmbedTextures(true);
    
    if (!serializer.Serialize(asset, file)) {
        SE_LOG_ERROR("MaterialSerializer: Failed to serialize material");
        return false;
    }
    
    SE_LOG_INFO("MaterialSerializer: Material saved successfully with embedded textures");
    return true;
}

bool MaterialSerializer::Load(EditorMaterialData& data, const std::filesystem::path& path) {
    SE_LOG_INFO("MaterialSerializer: Loading material from {}", path.string());
    
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        SE_LOG_ERROR("MaterialSerializer: Failed to open file for reading: {}", path.string());
        return false;
    }
    
    // Peek at magic to determine format version
    uint32_t magic = 0;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    file.seekg(0);
    
    if (magic == se::MaterialAsset::MAGIC) {
        // V2 format with embedded textures
        se::MaterialAsset asset;
        se::MaterialAssetSerializer serializer;
        
        if (!serializer.Deserialize(asset, file)) {
            SE_LOG_ERROR("MaterialSerializer: Failed to deserialize V2 material");
            return false;
        }
        
        data.FromEngineAsset(asset);
        data.filePath = path.string();
        data.isDirty = false;
        
        SE_LOG_INFO("MaterialSerializer: Loaded V2 material '{}' (embedded: {})",
                    data.name, asset.HasEmbeddedTextures());
        return true;
    }
    
    if (magic == MAGIC) {
        // Legacy V1 format
        return LoadV1(data, path);
    }
    
    SE_LOG_ERROR("MaterialSerializer: Unknown file format (magic: 0x{:08X})", magic);
    return false;
}

bool MaterialSerializer::LoadV1(EditorMaterialData& data, const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        SE_LOG_ERROR("MaterialSerializer: Failed to open V1 file: {}", path.string());
        return false;
    }
    
    SE_LOG_INFO("MaterialSerializer: Loading legacy V1 material from {}", path.string());
    
    uint32_t magic = 0;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != MAGIC) {
        SE_LOG_ERROR("MaterialSerializer: Invalid V1 magic");
        return false;
    }
    
    uint32_t version = 0;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version > VERSION) {
        SE_LOG_ERROR("MaterialSerializer: Unsupported V1 version {}", version);
        return false;
    }
    
    if (!ReadString(file, data.name)) return false;
    if (!ReadString(file, data.uuid)) return false;
    
    file.read(reinterpret_cast<char*>(&data.baseColor), sizeof(data.baseColor));
    file.read(reinterpret_cast<char*>(&data.metallic), sizeof(data.metallic));
    file.read(reinterpret_cast<char*>(&data.roughness), sizeof(data.roughness));
    file.read(reinterpret_cast<char*>(&data.reflectance), sizeof(data.reflectance));
    file.read(reinterpret_cast<char*>(&data.ao), sizeof(data.ao));
    file.read(reinterpret_cast<char*>(&data.normalScale), sizeof(data.normalScale));
    
    file.read(reinterpret_cast<char*>(&data.emissiveColor), sizeof(data.emissiveColor));
    file.read(reinterpret_cast<char*>(&data.emissiveFactor), sizeof(data.emissiveFactor));
    
    file.read(reinterpret_cast<char*>(&data.clearCoat), sizeof(data.clearCoat));
    file.read(reinterpret_cast<char*>(&data.clearCoatRoughness), sizeof(data.clearCoatRoughness));
    
    file.read(reinterpret_cast<char*>(&data.anisotropy), sizeof(data.anisotropy));
    file.read(reinterpret_cast<char*>(&data.anisotropyDirection), sizeof(data.anisotropyDirection));
    
    file.read(reinterpret_cast<char*>(&data.sheenColor), sizeof(data.sheenColor));
    file.read(reinterpret_cast<char*>(&data.sheenRoughness), sizeof(data.sheenRoughness));
    
    file.read(reinterpret_cast<char*>(&data.subsurfaceColor), sizeof(data.subsurfaceColor));
    file.read(reinterpret_cast<char*>(&data.subsurfacePower), sizeof(data.subsurfacePower));
    file.read(reinterpret_cast<char*>(&data.thickness), sizeof(data.thickness));
    
    file.read(reinterpret_cast<char*>(&data.transmission), sizeof(data.transmission));
    file.read(reinterpret_cast<char*>(&data.ior), sizeof(data.ior));
    
    if (!ReadString(file, data.albedoTexturePath)) return false;
    if (!ReadString(file, data.normalTexturePath)) return false;
    if (!ReadString(file, data.metallicTexturePath)) return false;
    if (!ReadString(file, data.roughnessTexturePath)) return false;
    if (!ReadString(file, data.aoTexturePath)) return false;
    if (!ReadString(file, data.emissiveTexturePath)) return false;
    
    uint8_t textureFlags = 0;
    file.read(reinterpret_cast<char*>(&textureFlags), sizeof(textureFlags));
    data.useAlbedoTexture = (textureFlags & (1 << 0)) != 0;
    data.useNormalTexture = (textureFlags & (1 << 1)) != 0;
    data.useMetallicTexture = (textureFlags & (1 << 2)) != 0;
    data.useRoughnessTexture = (textureFlags & (1 << 3)) != 0;
    data.useAOTexture = (textureFlags & (1 << 4)) != 0;
    data.useEmissiveTexture = (textureFlags & (1 << 5)) != 0;
    
    data.filePath = path.string();
    data.isDirty = false;
    
    SE_LOG_INFO("MaterialSerializer: Loaded V1 material '{}' (will upgrade on save)", data.name);
    return true;
}

void MaterialSerializer::WriteString(std::ofstream& file, const std::string& str) {
    uint32_t length = static_cast<uint32_t>(str.size());
    file.write(reinterpret_cast<const char*>(&length), sizeof(length));
    if (length > 0) {
        file.write(str.data(), length);
    }
}

bool MaterialSerializer::ReadString(std::ifstream& file, std::string& str) {
    uint32_t length = 0;
    file.read(reinterpret_cast<char*>(&length), sizeof(length));
    if (file.fail()) return false;
    
    if (length > 0) {
        str.resize(length);
        file.read(str.data(), length);
        if (file.fail()) return false;
    } else {
        str.clear();
    }
    return true;
}

}  // namespace mst
