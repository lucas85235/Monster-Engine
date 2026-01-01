#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "engine/renderer/MaterialDefinition.h"

namespace se {

class MaterialInstance;
class Texture;

class MaterialLibrary {
public:
    static MaterialLibrary& Get();
    
    static void Init();
    static void Shutdown();
    
    MaterialInstance* Load(const std::string& path);
    
    MaterialInstance* Get(const std::string& name);
    
    MaterialInstance* GetOrCreate(const MaterialDefinition& definition);
    
    MaterialInstance* CreateFromTextureMaterial(
        const std::string& name,
        std::shared_ptr<Texture> albedo,
        std::shared_ptr<Texture> normal = nullptr,
        std::shared_ptr<Texture> metallic = nullptr,
        std::shared_ptr<Texture> roughness = nullptr,
        std::shared_ptr<Texture> ao = nullptr,
        std::shared_ptr<Texture> emissive = nullptr
    );
    
    MaterialInstance* GetDefault();
    
    void Unload(const std::string& name);
    void ReloadAll();
    void Clear();
    
    size_t GetMaterialCount() const { return materials_.size(); }
    std::vector<std::string> GetMaterialNames() const;
    std::vector<MaterialInstance*> GetAllMaterials();
    
    void MarkDirty(const std::string& name);
    void SaveAll();
    
private:
    MaterialLibrary() = default;
    ~MaterialLibrary();
    
    MaterialLibrary(const MaterialLibrary&) = delete;
    MaterialLibrary& operator=(const MaterialLibrary&) = delete;
    
    void CreateDefaultMaterial();
    
    std::unordered_map<std::string, std::shared_ptr<MaterialInstance>> materials_;
    std::shared_ptr<MaterialInstance> defaultMaterial_;
    
    static MaterialLibrary* instance_;
    static bool initialized_;
};

}  // namespace se
