#pragma once

#include <filesystem>
#include <string>

#include "core/MapData.h"

namespace mst {

class MapSerializer {
   public:
    static bool Export(const MapData& data, const std::filesystem::path& path);

   private:
    static void WriteString(std::ofstream& file, const std::string& str);
    static void WriteHeader(std::ofstream& file, const MapData& data);
    static void WriteEntity(std::ofstream& file, const MapEntityData& entity);
};

}  // namespace mst
