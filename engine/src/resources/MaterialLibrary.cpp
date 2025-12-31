#include "engine/resources/MaterialLibrary.h"

#include "engine/Log.h"
#include "engine/renderer/MaterialInstance.h"
#include "engine/renderer/Texture.h"

namespace se {

MaterialLibrary* MaterialLibrary::instance_ = nullptr;
bool MaterialLibrary::initialized_ = false;

MaterialLibrary& MaterialLibrary::Get() {
    if (!instance_) {
        instance_ = new MaterialLibrary();
    }
    return *instance_;
}

void MaterialLibrary::Init() {
    if (initialized_) {
        SE_LOG_WARN("MaterialLibrary already initialized");
        return;
    }
    
    SE_LOG_INFO("Initializing MaterialLibrary");
    Get().CreateDefaultMaterial();
    initialized_ = true;
}

void MaterialLibrary::Shutdown() {
    if (!initialized_) return;
    
    SE_LOG_INFO("Shutting down MaterialLibrary");
    if (instance_) {
        instance_->Clear();
        delete instance_;
        instance_ = nullptr;
    }
    initialized_ = false;
}

MaterialLibrary::~MaterialLibrary() {
    Clear();
}

void MaterialLibrary::CreateDefaultMaterial() {
    MaterialDefinition defaultDef;
    defaultDef.name = "Default";
    defaultDef.baseColor = Vector4(0.7f, 0.7f, 0.7f, 1.0f);
    defaultDef.metallic = 0.0f;
    defaultDef.roughness = 0.5f;
    
    defaultMaterial_ = MaterialInstance::Create(defaultDef);
    SE_LOG_INFO("Created default material");
}

MaterialInstance* MaterialLibrary::Load(const std::string& path) {
    auto it = materials_.find(path);
    if (it != materials_.end()) {
        return it->second.get();
    }
    
    MaterialDefinition def;
    def.name = path;
    def.sourcePath = path;
    
    auto instance = MaterialInstance::Create(def);
    materials_[path] = instance;
    
    SE_LOG_INFO("MaterialLibrary: loaded material '{}'", path);
    return instance.get();
}

MaterialInstance* MaterialLibrary::Get(const std::string& name) {
    auto it = materials_.find(name);
    if (it != materials_.end()) {
        return it->second.get();
    }
    return nullptr;
}

MaterialInstance* MaterialLibrary::GetOrCreate(const MaterialDefinition& definition) {
    const std::string& key = definition.sourcePath.empty() ? definition.name : definition.sourcePath;
    
    auto it = materials_.find(key);
    if (it != materials_.end()) {
        return it->second.get();
    }
    
    auto instance = MaterialInstance::Create(definition);
    materials_[key] = instance;
    
    SE_LOG_INFO("MaterialLibrary: created material '{}'", definition.name);
    return instance.get();
}

MaterialInstance* MaterialLibrary::CreateFromTextureMaterial(
    const std::string& name,
    std::shared_ptr<Texture> albedo,
    std::shared_ptr<Texture> normal,
    std::shared_ptr<Texture> metallic,
    std::shared_ptr<Texture> roughness,
    std::shared_ptr<Texture> ao,
    std::shared_ptr<Texture> emissive
) {
    auto it = materials_.find(name);
    if (it != materials_.end()) {
        return it->second.get();
    }
    
    auto instance = MaterialInstance::CreateFromTextures(
        name, albedo, normal, metallic, roughness, ao, emissive
    );
    materials_[name] = instance;
    
    SE_LOG_INFO("MaterialLibrary: created material from textures '{}'", name);
    return instance.get();
}

MaterialInstance* MaterialLibrary::GetDefault() {
    if (!defaultMaterial_) {
        CreateDefaultMaterial();
    }
    return defaultMaterial_.get();
}

void MaterialLibrary::Unload(const std::string& name) {
    auto it = materials_.find(name);
    if (it != materials_.end()) {
        SE_LOG_INFO("MaterialLibrary: unloading material '{}'", name);
        materials_.erase(it);
    }
}

void MaterialLibrary::ReloadAll() {
    SE_LOG_INFO("MaterialLibrary: reloading all {} materials", materials_.size());
    for (auto& [name, mat] : materials_) {
        mat->Reload();
    }
    if (defaultMaterial_) {
        defaultMaterial_->Reload();
    }
}

void MaterialLibrary::Clear() {
    SE_LOG_INFO("MaterialLibrary: clearing {} materials", materials_.size());
    materials_.clear();
    defaultMaterial_.reset();
}

std::vector<std::string> MaterialLibrary::GetMaterialNames() const {
    std::vector<std::string> names;
    names.reserve(materials_.size());
    for (const auto& [name, _] : materials_) {
        names.push_back(name);
    }
    return names;
}

std::vector<MaterialInstance*> MaterialLibrary::GetAllMaterials() {
    std::vector<MaterialInstance*> result;
    result.reserve(materials_.size());
    for (auto& [_, mat] : materials_) {
        result.push_back(mat.get());
    }
    return result;
}

void MaterialLibrary::MarkDirty(const std::string& name) {
    auto it = materials_.find(name);
    if (it != materials_.end()) {
        it->second->MarkDirty();
    }
}

void MaterialLibrary::SaveAll() {
    SE_LOG_INFO("MaterialLibrary: SaveAll not yet implemented (serialization pending)");
}

}  // namespace se
