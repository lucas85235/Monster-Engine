#pragma once

#include <memory>
#include <string>

#include "engine/resources/ModelData.h"

namespace se {

// Loads models with skeleton data for animation
class SkinnedModelLoader {
public:
    static std::unique_ptr<SkinnedModelData> Load(const std::string& path);
    
private:
    static void ProcessNode(const void* node, const void* scene, SkinnedModelData& data, const std::string& directory);
    static void ProcessMesh(const void* mesh, const void* scene, SkinnedModelData& data);
    static void ExtractBones(const void* mesh, SkinnedModelData& data, std::vector<SkinnedVertex>& vertices);
    static void ProcessBoneHierarchy(const void* node, SkinnedModelData& data, int parentIndex);
    static void ProcessMaterial(const void* material, SkinnedModelData& data, const std::string& directory);
};

}  // namespace se
