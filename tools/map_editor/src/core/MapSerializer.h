#pragma once
/**
 * MapSerializer.h - Binary map file I/O.
 *
 * Handles reading/writing .mstmap files with versioned format.
 * Supports entity transforms, collision settings, and player start position.
 */

#include <filesystem>
#include <string>

#include "core/MapData.h"

namespace mst {

class MapSerializer {
   public:
    static bool Export(const MapData& data, const std::filesystem::path& path);
    static bool Import(MapData& data, const std::filesystem::path& path);

   private:
    static void WriteString(std::ofstream& file, const std::string& str);
    static void WriteHeader(std::ofstream& file, const MapData& data);
    static void WriteEntity(std::ofstream& file, const MapEntityData& entity);
    
    static bool ReadString(std::ifstream& file, std::string& str);
    static bool ReadHeader(std::ifstream& file, MapData& data, uint32_t& entityCount, uint32_t& version);
    static bool ReadEntity(std::ifstream& file, MapEntityData& entity, uint32_t version);
};

}  // namespace mst
