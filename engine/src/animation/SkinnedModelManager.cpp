#include "engine/animation/SkinnedModelManager.h"

#include <filesystem>

#include "engine/animation/SkinnedMesh.h"
#include "engine/animation/SkinnedModel.h"
#include "engine/animation/SkinnedModelLoader.h"
#include "engine/Log.h"
#include "engine/renderer/TextureMaterial.h"
#include "engine/resources/TextureManager.h"

namespace se {

std::unordered_map<std::string, std::shared_ptr<SkinnedModel>> SkinnedModelManager::cache_;
bool SkinnedModelManager::initialized_ = false;

void SkinnedModelManager::Init() {
    if (initialized_) return;
    initialized_ = true;
    SE_LOG_INFO("SkinnedModelManager: Initialized");
}

void SkinnedModelManager::Shutdown() {
    if (!initialized_) return;
    ClearCache();
    initialized_ = false;
    SE_LOG_INFO("SkinnedModelManager: Shutdown");
}

std::shared_ptr<SkinnedModel> SkinnedModelManager::Load(const std::string& path) {
    if (!initialized_) Init();
    
    std::filesystem::path filePath(path);
    std::string name = filePath.stem().string();
    
    auto it = cache_.find(name);
    if (it != cache_.end()) {
        SE_LOG_INFO("SkinnedModelManager: Cache hit for '{}'", name);
        return it->second;
    }
    
    auto data = SkinnedModelLoader::Load(path);
    if (!data) {
        SE_LOG_ERROR("SkinnedModelManager: Failed to load '{}'", path);
        return nullptr;
    }
    
    auto sharedData = std::shared_ptr<SkinnedModelData>(std::move(data));
    auto model = CreateFromData(sharedData);
    
    if (model) {
        cache_[name] = model;
        SE_LOG_INFO("SkinnedModelManager: Loaded and cached '{}'", name);
    }
    
    return model;
}

std::shared_ptr<SkinnedModel> SkinnedModelManager::Get(const std::string& name) {
    auto it = cache_.find(name);
    if (it != cache_.end()) {
        return it->second;
    }
    return nullptr;
}

bool SkinnedModelManager::Has(const std::string& name) {
    return cache_.find(name) != cache_.end();
}

void SkinnedModelManager::Unload(const std::string& name) {
    auto it = cache_.find(name);
    if (it != cache_.end()) {
        cache_.erase(it);
        SE_LOG_INFO("SkinnedModelManager: Unloaded '{}'", name);
    }
}

void SkinnedModelManager::ClearCache() {
    size_t count = cache_.size();
    cache_.clear();
    SE_LOG_INFO("SkinnedModelManager: Cleared {} cached models", count);
}

std::shared_ptr<SkinnedModel> SkinnedModelManager::CreateFromData(std::shared_ptr<SkinnedModelData> data) {
    auto model = std::make_shared<SkinnedModel>(data->Name);
    model->SetModelData(data);
    model->SetBoundingBox(data->Bounds);
    
    std::filesystem::path sourcePath(data->SourcePath);
    std::string directory = sourcePath.parent_path().string();
    
    for (size_t i = 0; i < data->SubMeshes.size(); ++i) {
        SkinnedMesh mesh;
        mesh.Create(data->SubMeshes[i]);
        
        // Create material if available
        int matIndex = data->SubMeshes[i].MaterialIndex;
        if (matIndex >= 0 && matIndex < static_cast<int>(data->Materials.size())) {
            const MaterialData& matData = data->Materials[matIndex];
            auto material = std::make_shared<TextureMaterial>();
            
            if (!matData.DiffuseTexturePath.empty()) {
                material->Albedo = TextureManager::Load(matData.DiffuseTexturePath);
            }
            if (!matData.NormalTexturePath.empty()) {
                material->Normal = TextureManager::Load(matData.NormalTexturePath);
            }
            if (!matData.SpecularTexturePath.empty()) {
                material->Specular = TextureManager::Load(matData.SpecularTexturePath);
            }
            if (!matData.AOTexturePath.empty()) {
                material->AO = TextureManager::Load(matData.AOTexturePath);
            }
            
            material->BaseColor = matData.DiffuseColor;
            mesh.SetMaterial(material);
        }
        
        model->AddMesh(std::move(mesh));
    }
    
    SE_LOG_INFO("SkinnedModelManager: Created model '{}' with {} meshes, {} bones",
                data->Name, model->GetMeshCount(), data->Bones.size());
    
    return model;
}

}  // namespace se
