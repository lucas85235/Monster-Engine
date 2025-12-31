#pragma once
/**
 * MaterialSerializer.h - Binary material file I/O.
 *
 * Handles reading/writing .mstmat files with versioned format.
 * Supports all PBR parameters and texture path references.
 */

#include <filesystem>
#include <string>

#include "core/EditorMaterialData.h"

namespace mst {

class MaterialSerializer {
   public:
    static constexpr uint32_t MAGIC   = 0x4D535441;  // "MSTA" (Monster Material V1)
    static constexpr uint32_t VERSION = 1;

    static bool Save(const EditorMaterialData& data, const std::filesystem::path& path);
    static bool Load(EditorMaterialData& data, const std::filesystem::path& path);

   private:
    static bool LoadV1(EditorMaterialData& data, const std::filesystem::path& path);
    static void WriteString(std::ofstream& file, const std::string& str);
    static bool ReadString(std::ifstream& file, std::string& str);
};

}  // namespace mst
