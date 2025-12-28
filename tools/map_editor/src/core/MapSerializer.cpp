#include "core/MapSerializer.h"

#include <fstream>

#include "engine/Log.h"

namespace mst {

bool MapSerializer::Export(const MapData& data, const std::filesystem::path& path) {
    SE_LOG_INFO("MapSerializer: Exporting map '{}' to '{}'", data.mapName, path.string());

    std::ofstream file(path, std::ios::binary);
    if (!file.is_open()) {
        SE_LOG_ERROR("MapSerializer: Failed to open file for writing: {}", path.string());
        return false;
    }

    WriteHeader(file, data);
    WriteString(file, data.mapName);

    for (const auto& entity : data.entities) {
        WriteEntity(file, entity);
    }

    file.close();

    if (file.fail()) {
        SE_LOG_ERROR("MapSerializer: Write operation failed for: {}", path.string());
        return false;
    }

    SE_LOG_INFO("MapSerializer: Successfully exported {} entities to '{}'", data.entities.size(),
                path.string());
    return true;
}

void MapSerializer::WriteString(std::ofstream& file, const std::string& str) {
    uint32_t length = static_cast<uint32_t>(str.size());
    file.write(reinterpret_cast<const char*>(&length), sizeof(length));
    file.write(str.data(), length);
}

void MapSerializer::WriteHeader(std::ofstream& file, const MapData& data) {
    file.write(reinterpret_cast<const char*>(&MapData::MAGIC), sizeof(MapData::MAGIC));
    file.write(reinterpret_cast<const char*>(&MapData::VERSION), sizeof(MapData::VERSION));

    uint32_t entityCount = static_cast<uint32_t>(data.entities.size());
    file.write(reinterpret_cast<const char*>(&entityCount), sizeof(entityCount));

    // Player Start data
    uint8_t hasPlayerStart = data.hasPlayerStart ? 1 : 0;
    file.write(reinterpret_cast<const char*>(&hasPlayerStart), sizeof(hasPlayerStart));
    file.write(reinterpret_cast<const char*>(&data.playerStartPosition), sizeof(Vector3));
    file.write(reinterpret_cast<const char*>(&data.playerStartRotation), sizeof(Vector3));
    
    // Directional Light data (version 3+)
    uint8_t hasLight = data.hasDirectionalLight ? 1 : 0;
    file.write(reinterpret_cast<const char*>(&hasLight), sizeof(hasLight));
    if (data.hasDirectionalLight) {
        file.write(reinterpret_cast<const char*>(&data.directionalLight.direction), sizeof(Vector3));
        file.write(reinterpret_cast<const char*>(&data.directionalLight.position), sizeof(Vector3));
        file.write(reinterpret_cast<const char*>(&data.directionalLight.rotation), sizeof(Vector3));
        file.write(reinterpret_cast<const char*>(&data.directionalLight.color), sizeof(Vector3));
        file.write(reinterpret_cast<const char*>(&data.directionalLight.intensity), sizeof(float));
        uint8_t castShadows = data.directionalLight.castShadows ? 1 : 0;
        file.write(reinterpret_cast<const char*>(&castShadows), sizeof(castShadows));
        uint8_t enabled = data.directionalLight.enabled ? 1 : 0;
        file.write(reinterpret_cast<const char*>(&enabled), sizeof(enabled));
    }
}

void MapSerializer::WriteEntity(std::ofstream& file, const MapEntityData& entity) {
    WriteString(file, entity.name);

    file.write(reinterpret_cast<const char*>(&entity.primitiveType), sizeof(entity.primitiveType));

    file.write(reinterpret_cast<const char*>(&entity.position), sizeof(Vector3));
    file.write(reinterpret_cast<const char*>(&entity.rotation), sizeof(Vector3));
    file.write(reinterpret_cast<const char*>(&entity.scale), sizeof(Vector3));
    file.write(reinterpret_cast<const char*>(&entity.color), sizeof(Vector4));

    uint8_t hasCollision = entity.hasCollision ? 1 : 0;
    file.write(reinterpret_cast<const char*>(&hasCollision), sizeof(hasCollision));

    if (entity.hasCollision) {
        file.write(reinterpret_cast<const char*>(&entity.colliderType),
                   sizeof(entity.colliderType));
        file.write(reinterpret_cast<const char*>(&entity.colliderSize), sizeof(Vector3));
        file.write(reinterpret_cast<const char*>(&entity.colliderRadius), sizeof(float));
        file.write(reinterpret_cast<const char*>(&entity.colliderHeight), sizeof(float));
        
        // Rigidbody settings
        file.write(reinterpret_cast<const char*>(&entity.rigidbodyType), sizeof(entity.rigidbodyType));
        file.write(reinterpret_cast<const char*>(&entity.mass), sizeof(float));
    }
    
    // Emissive properties (version 4+)
    file.write(reinterpret_cast<const char*>(&entity.emissiveColor), sizeof(Vector3));
    file.write(reinterpret_cast<const char*>(&entity.emissiveFactor), sizeof(float));
}

bool MapSerializer::Import(MapData& data, const std::filesystem::path& path) {
    SE_LOG_INFO("MapSerializer: Importing map from '{}'", path.string());

    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        SE_LOG_ERROR("MapSerializer: Failed to open file for reading: {}", path.string());
        return false;
    }

    uint32_t entityCount = 0;
    uint32_t version = 0;
    if (!ReadHeader(file, data, entityCount, version)) {
        SE_LOG_ERROR("MapSerializer: Failed to read header from: {}", path.string());
        return false;
    }

    if (!ReadString(file, data.mapName)) {
        SE_LOG_ERROR("MapSerializer: Failed to read map name from: {}", path.string());
        return false;
    }

    data.entities.clear();
    data.entities.reserve(entityCount);

    for (uint32_t i = 0; i < entityCount; ++i) {
        MapEntityData entity;
        if (!ReadEntity(file, entity, version)) {
            SE_LOG_ERROR("MapSerializer: Failed to read entity {} from: {}", i, path.string());
            return false;
        }
        data.entities.push_back(std::move(entity));
    }

    file.close();

    SE_LOG_INFO("MapSerializer: Successfully imported {} entities from '{}'", 
                data.entities.size(), path.string());
    return true;
}

bool MapSerializer::ReadString(std::ifstream& file, std::string& str) {
    uint32_t length = 0;
    file.read(reinterpret_cast<char*>(&length), sizeof(length));
    if (file.fail() || length > 10000) return false;  // Sanity check
    
    str.resize(length);
    file.read(str.data(), length);
    return !file.fail();
}

bool MapSerializer::ReadHeader(std::ifstream& file, MapData& data, uint32_t& entityCount, uint32_t& version) {
    uint32_t magic = 0;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != MapData::MAGIC) {
        SE_LOG_ERROR("MapSerializer: Invalid file magic: expected 0x{:08X}, got 0x{:08X}", 
                     MapData::MAGIC, magic);
        return false;
    }

    version = 0;
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    
    file.read(reinterpret_cast<char*>(&entityCount), sizeof(entityCount));
    
    // Version 2+ has Player Start data
    if (version >= 2) {
        uint8_t hasPlayerStart = 0;
        file.read(reinterpret_cast<char*>(&hasPlayerStart), sizeof(hasPlayerStart));
        data.hasPlayerStart = (hasPlayerStart != 0);
        file.read(reinterpret_cast<char*>(&data.playerStartPosition), sizeof(Vector3));
        file.read(reinterpret_cast<char*>(&data.playerStartRotation), sizeof(Vector3));
    } else {
        // Version 1 had a reserved field
        uint32_t reserved = 0;
        file.read(reinterpret_cast<char*>(&reserved), sizeof(reserved));
        data.hasPlayerStart = false;
    }
    
    // Version 3+ has Directional Light data
    if (version >= 3) {
        uint8_t hasLight = 0;
        file.read(reinterpret_cast<char*>(&hasLight), sizeof(hasLight));
        data.hasDirectionalLight = (hasLight != 0);
        if (data.hasDirectionalLight) {
            file.read(reinterpret_cast<char*>(&data.directionalLight.direction), sizeof(Vector3));
            file.read(reinterpret_cast<char*>(&data.directionalLight.position), sizeof(Vector3));
            file.read(reinterpret_cast<char*>(&data.directionalLight.rotation), sizeof(Vector3));
            file.read(reinterpret_cast<char*>(&data.directionalLight.color), sizeof(Vector3));
            file.read(reinterpret_cast<char*>(&data.directionalLight.intensity), sizeof(float));
            uint8_t castShadows = 0;
            file.read(reinterpret_cast<char*>(&castShadows), sizeof(castShadows));
            data.directionalLight.castShadows = (castShadows != 0);
            uint8_t enabled = 0;
            file.read(reinterpret_cast<char*>(&enabled), sizeof(enabled));
            data.directionalLight.enabled = (enabled != 0);
        }
    } else {
        // Default light for older maps
        data.hasDirectionalLight = true;
        data.directionalLight = MapDirectionalLightData{};
    }

    return !file.fail();
}

bool MapSerializer::ReadEntity(std::ifstream& file, MapEntityData& entity, uint32_t version) {
    if (!ReadString(file, entity.name)) return false;

    file.read(reinterpret_cast<char*>(&entity.primitiveType), sizeof(entity.primitiveType));

    file.read(reinterpret_cast<char*>(&entity.position), sizeof(Vector3));
    file.read(reinterpret_cast<char*>(&entity.rotation), sizeof(Vector3));
    file.read(reinterpret_cast<char*>(&entity.scale), sizeof(Vector3));
    file.read(reinterpret_cast<char*>(&entity.color), sizeof(Vector4));

    uint8_t hasCollision = 0;
    file.read(reinterpret_cast<char*>(&hasCollision), sizeof(hasCollision));
    entity.hasCollision = (hasCollision != 0);

    if (entity.hasCollision) {
        file.read(reinterpret_cast<char*>(&entity.colliderType), sizeof(entity.colliderType));
        file.read(reinterpret_cast<char*>(&entity.colliderSize), sizeof(Vector3));
        file.read(reinterpret_cast<char*>(&entity.colliderRadius), sizeof(float));
        file.read(reinterpret_cast<char*>(&entity.colliderHeight), sizeof(float));
        
        // Rigidbody settings
        file.read(reinterpret_cast<char*>(&entity.rigidbodyType), sizeof(entity.rigidbodyType));
        file.read(reinterpret_cast<char*>(&entity.mass), sizeof(float));
    }
    
    // Emissive properties (version 4+)
    if (version >= 4) {
        file.read(reinterpret_cast<char*>(&entity.emissiveColor), sizeof(Vector3));
        file.read(reinterpret_cast<char*>(&entity.emissiveFactor), sizeof(float));
    } else {
        // Default values for older maps
        entity.emissiveColor = Vector3{0.0f, 0.0f, 0.0f};
        entity.emissiveFactor = 0.0f;
    }

    return !file.fail();
}

}  // namespace mst

