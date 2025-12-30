#include "core/MaterialSerializer.h"

#include <fstream>

#include "engine/Log.h"

namespace mst {

bool MaterialSerializer::Save(const EditorMaterialData& data, const std::filesystem::path& path) {
    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        SE_LOG_ERROR("MaterialSerializer: Failed to open file for writing: {}", path.string());
        return false;
    }

    SE_LOG_INFO("MaterialSerializer: Saving material '{}' to {}", data.name, path.string());

    // Header
    file.write(reinterpret_cast<const char*>(&MAGIC), sizeof(MAGIC));
    file.write(reinterpret_cast<const char*>(&VERSION), sizeof(VERSION));

    // Material name and UUID
    WriteString(file, data.name);
    WriteString(file, data.uuid);

    // Core PBR parameters
    file.write(reinterpret_cast<const char*>(&data.baseColor), sizeof(data.baseColor));
    file.write(reinterpret_cast<const char*>(&data.metallic), sizeof(data.metallic));
    file.write(reinterpret_cast<const char*>(&data.roughness), sizeof(data.roughness));
    file.write(reinterpret_cast<const char*>(&data.reflectance), sizeof(data.reflectance));
    file.write(reinterpret_cast<const char*>(&data.ao), sizeof(data.ao));
    file.write(reinterpret_cast<const char*>(&data.normalScale), sizeof(data.normalScale));

    // Emissive
    file.write(reinterpret_cast<const char*>(&data.emissiveColor), sizeof(data.emissiveColor));
    file.write(reinterpret_cast<const char*>(&data.emissiveFactor), sizeof(data.emissiveFactor));

    // Clear coat
    file.write(reinterpret_cast<const char*>(&data.clearCoat), sizeof(data.clearCoat));
    file.write(reinterpret_cast<const char*>(&data.clearCoatRoughness), sizeof(data.clearCoatRoughness));

    // Anisotropy
    file.write(reinterpret_cast<const char*>(&data.anisotropy), sizeof(data.anisotropy));
    file.write(reinterpret_cast<const char*>(&data.anisotropyDirection), sizeof(data.anisotropyDirection));

    // Sheen
    file.write(reinterpret_cast<const char*>(&data.sheenColor), sizeof(data.sheenColor));
    file.write(reinterpret_cast<const char*>(&data.sheenRoughness), sizeof(data.sheenRoughness));

    // Subsurface
    file.write(reinterpret_cast<const char*>(&data.subsurfaceColor), sizeof(data.subsurfaceColor));
    file.write(reinterpret_cast<const char*>(&data.subsurfacePower), sizeof(data.subsurfacePower));
    file.write(reinterpret_cast<const char*>(&data.thickness), sizeof(data.thickness));

    // Transmission
    file.write(reinterpret_cast<const char*>(&data.transmission), sizeof(data.transmission));
    file.write(reinterpret_cast<const char*>(&data.ior), sizeof(data.ior));

    // Texture paths
    WriteString(file, data.albedoTexturePath);
    WriteString(file, data.normalTexturePath);
    WriteString(file, data.metallicTexturePath);
    WriteString(file, data.roughnessTexturePath);
    WriteString(file, data.aoTexturePath);
    WriteString(file, data.emissiveTexturePath);

    // Texture usage flags
    uint8_t textureFlags = 0;
    if (data.useAlbedoTexture) textureFlags |= (1 << 0);
    if (data.useNormalTexture) textureFlags |= (1 << 1);
    if (data.useMetallicTexture) textureFlags |= (1 << 2);
    if (data.useRoughnessTexture) textureFlags |= (1 << 3);
    if (data.useAOTexture) textureFlags |= (1 << 4);
    if (data.useEmissiveTexture) textureFlags |= (1 << 5);
    file.write(reinterpret_cast<const char*>(&textureFlags), sizeof(textureFlags));

    SE_LOG_INFO("MaterialSerializer: Material saved successfully");
    return true;
}

bool MaterialSerializer::Load(EditorMaterialData& data, const std::filesystem::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        SE_LOG_ERROR("MaterialSerializer: Failed to open file for reading: {}", path.string());
        return false;
    }

    SE_LOG_INFO("MaterialSerializer: Loading material from {}", path.string());

    // Verify header
    uint32_t magic = 0;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != MAGIC) {
        SE_LOG_ERROR("MaterialSerializer: Invalid file format (magic mismatch)");
        return false;
    }

    uint32_t version = 0;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    if (version > VERSION) {
        SE_LOG_ERROR("MaterialSerializer: Unsupported version {} (max supported: {})", version, VERSION);
        return false;
    }

    // Material name and UUID
    if (!ReadString(file, data.name)) return false;
    if (!ReadString(file, data.uuid)) return false;

    // Core PBR parameters
    file.read(reinterpret_cast<char*>(&data.baseColor), sizeof(data.baseColor));
    file.read(reinterpret_cast<char*>(&data.metallic), sizeof(data.metallic));
    file.read(reinterpret_cast<char*>(&data.roughness), sizeof(data.roughness));
    file.read(reinterpret_cast<char*>(&data.reflectance), sizeof(data.reflectance));
    file.read(reinterpret_cast<char*>(&data.ao), sizeof(data.ao));
    file.read(reinterpret_cast<char*>(&data.normalScale), sizeof(data.normalScale));

    // Emissive
    file.read(reinterpret_cast<char*>(&data.emissiveColor), sizeof(data.emissiveColor));
    file.read(reinterpret_cast<char*>(&data.emissiveFactor), sizeof(data.emissiveFactor));

    // Clear coat
    file.read(reinterpret_cast<char*>(&data.clearCoat), sizeof(data.clearCoat));
    file.read(reinterpret_cast<char*>(&data.clearCoatRoughness), sizeof(data.clearCoatRoughness));

    // Anisotropy
    file.read(reinterpret_cast<char*>(&data.anisotropy), sizeof(data.anisotropy));
    file.read(reinterpret_cast<char*>(&data.anisotropyDirection), sizeof(data.anisotropyDirection));

    // Sheen
    file.read(reinterpret_cast<char*>(&data.sheenColor), sizeof(data.sheenColor));
    file.read(reinterpret_cast<char*>(&data.sheenRoughness), sizeof(data.sheenRoughness));

    // Subsurface
    file.read(reinterpret_cast<char*>(&data.subsurfaceColor), sizeof(data.subsurfaceColor));
    file.read(reinterpret_cast<char*>(&data.subsurfacePower), sizeof(data.subsurfacePower));
    file.read(reinterpret_cast<char*>(&data.thickness), sizeof(data.thickness));

    // Transmission
    file.read(reinterpret_cast<char*>(&data.transmission), sizeof(data.transmission));
    file.read(reinterpret_cast<char*>(&data.ior), sizeof(data.ior));

    // Texture paths
    if (!ReadString(file, data.albedoTexturePath)) return false;
    if (!ReadString(file, data.normalTexturePath)) return false;
    if (!ReadString(file, data.metallicTexturePath)) return false;
    if (!ReadString(file, data.roughnessTexturePath)) return false;
    if (!ReadString(file, data.aoTexturePath)) return false;
    if (!ReadString(file, data.emissiveTexturePath)) return false;

    // Texture usage flags
    uint8_t textureFlags = 0;
    file.read(reinterpret_cast<char*>(&textureFlags), sizeof(textureFlags));
    data.useAlbedoTexture = (textureFlags & (1 << 0)) != 0;
    data.useNormalTexture = (textureFlags & (1 << 1)) != 0;
    data.useMetallicTexture = (textureFlags & (1 << 2)) != 0;
    data.useRoughnessTexture = (textureFlags & (1 << 3)) != 0;
    data.useAOTexture = (textureFlags & (1 << 4)) != 0;
    data.useEmissiveTexture = (textureFlags & (1 << 5)) != 0;

    // Set file path and mark as clean
    data.filePath = path.string();
    data.isDirty = false;

    SE_LOG_INFO("MaterialSerializer: Loaded material '{}'", data.name);
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
