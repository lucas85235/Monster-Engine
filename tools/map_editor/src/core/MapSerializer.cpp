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

    uint32_t reserved = 0;
    file.write(reinterpret_cast<const char*>(&reserved), sizeof(reserved));
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
    }
}

}  // namespace mst
