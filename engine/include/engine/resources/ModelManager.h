#pragma once

#include <memory>
#include <string>
#include <unordered_map>

namespace se {

class Model;
class IModelLoader;
struct ModelData;
struct MaterialData;
struct TextureMaterial;

class ModelManager {
   public:
    ModelManager() = delete;

    static void Init();
    static void Shutdown();

    static std::shared_ptr<Model> Load(const std::string& path);
    static std::shared_ptr<Model> Get(const std::string& name);
    static bool Has(const std::string& name);
    static void Unload(const std::string& name);
    static void ClearCache();

   private:
    static std::shared_ptr<Model> CreateModelFromData(const ModelData& data);
    static std::shared_ptr<TextureMaterial> CreateTextureMaterial(const MaterialData& matData);

    static std::unique_ptr<IModelLoader> loader_;
    static std::unordered_map<std::string, std::shared_ptr<Model>> cache_;
    static bool initialized_;
};

}  // namespace se

