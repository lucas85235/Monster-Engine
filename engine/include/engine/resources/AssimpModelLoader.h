#pragma once

#include "engine/resources/IModelLoader.h"
#include "engine/resources/ModelData.h"

#include <string>
#include <vector>

struct aiNode;
struct aiMesh;
struct aiScene;
struct aiMaterial;

namespace se {

class AssimpModelLoader : public IModelLoader {
   public:
    AssimpModelLoader() = default;
    ~AssimpModelLoader() override = default;

    std::unique_ptr<ModelData> Load(const std::string& path) override;
    bool SupportsFormat(const std::string& extension) const override;

   private:
    void ProcessNode(const aiNode* node, const aiScene* scene, ModelData& modelData);
    SubMeshData ProcessMesh(const aiMesh* mesh, const aiScene* scene);
    MaterialData ProcessMaterial(const aiMaterial* material, const std::string& directory);
    
    std::string directory_;
};

}  // namespace se
