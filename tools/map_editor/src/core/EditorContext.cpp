#include "EditorContext.h"

#include <algorithm>
#include <filesystem>
#include <set>

#include "MaterialSerializer.h"
#include "engine/Log.h"
#include "engine/renderer/TextureMaterial.h"
#include "engine/resources/TextureManager.h"

using namespace se;

namespace mst {

EditorContext::EditorContext() {
    SE_LOG_INFO("EditorContext: Initializing...");
    
    commandSystem_ = se::CreateScope<CommandSystem>(eventBus_);
    sceneManager_ = se::CreateScope<SceneManager>(eventBus_);
    entityManager_ = se::CreateScope<EntityManager>(*sceneManager_, eventBus_);
    document_ = se::CreateScope<MapDocument>(*entityManager_, *sceneManager_, eventBus_);
    
    camera_.FocusOnPoint({0.0f, 0.0f, 0.0f});
    
    SE_LOG_INFO("EditorContext: Initialized successfully");
}

EditorContext::~EditorContext() {
    SE_LOG_INFO("EditorContext: Shutting down");
}

EditorMaterialData* EditorContext::GetMaterialByIndex(int index) {
    if (index < 0 || index >= static_cast<int>(materials_.size())) {
        return nullptr;
    }
    return &materials_[index];
}

EditorMaterialData* EditorContext::GetMaterialByName(const std::string& name) {
    for (auto& mat : materials_) {
        if (mat.name == name) {
            return &mat;
        }
    }
    return nullptr;
}

int EditorContext::GetMaterialIndex(const std::string& name) const {
    for (int i = 0; i < static_cast<int>(materials_.size()); ++i) {
        if (materials_[i].name == name) {
            return i;
        }
    }
    return -1;
}

void EditorContext::CreateNewMaterial() {
    EditorMaterialData mat;
    mat.name = "Material_" + std::to_string(nextMaterialId_++);
    mat.uuid = std::to_string(nextMaterialId_);
    mat.isDirty = true;
    materials_.push_back(std::move(mat));
    SE_LOG_INFO("EditorContext: Created new material '{}'", materials_.back().name);
}

void EditorContext::DuplicateMaterial(int index) {
    if (index < 0 || index >= static_cast<int>(materials_.size())) {
        return;
    }
    EditorMaterialData copy = materials_[index];
    copy.name = materials_[index].name + "_Copy";
    copy.uuid = std::to_string(nextMaterialId_++);
    copy.isDirty = true;
    copy.filePath.clear();
    materials_.push_back(std::move(copy));
    SE_LOG_INFO("EditorContext: Duplicated material '{}'", materials_.back().name);
}

void EditorContext::RemoveMaterial(int index) {
    if (index < 0 || index >= static_cast<int>(materials_.size())) {
        return;
    }
    SE_LOG_INFO("EditorContext: Removing material '{}'", materials_[index].name);
    materials_.erase(materials_.begin() + index);
}

bool EditorContext::LoadMaterial(const std::string& path) {
    EditorMaterialData mat;
    if (MaterialSerializer::Load(mat, path)) {
        mat.filePath = path;
        mat.isDirty = false;
        materials_.push_back(std::move(mat));
        return true;
    }
    return false;
}

bool EditorContext::SaveMaterial(int index, const std::string& path) {
    if (index < 0 || index >= static_cast<int>(materials_.size())) {
        return false;
    }
    if (MaterialSerializer::Save(materials_[index], path)) {
        materials_[index].filePath = path;
        materials_[index].isDirty = false;
        return true;
    }
    return false;
}

void EditorContext::ClearMaterials() {
    materials_.clear();
    SE_LOG_INFO("EditorContext: Cleared all materials");
}

void EditorContext::ApplyMaterialsToLoadedEntities() {
    auto& scene = sceneManager_->GetScene();
    auto view = scene.GetAllEntitiesWith<se::MeshRenderComponent, PrimitiveFactory::EditorMetadata>();
    
    int appliedCount = 0;
    for (auto entityHandle : view) {
        se::Entity entity(entityHandle, sceneManager_->GetScenePtr());
        auto& metadata = entity.GetComponent<PrimitiveFactory::EditorMetadata>();
        
        if (!metadata.hasCustomMaterial || metadata.materialName.empty()) {
            continue;
        }
        
        // Find the material by name
        EditorMaterialData* mat = GetMaterialByName(metadata.materialName);
        if (!mat) {
            SE_LOG_WARN("EditorContext: Material '{}' not found for entity", metadata.materialName);
            continue;
        }
        
        auto& meshRender = entity.GetComponent<se::MeshRenderComponent>();
        
        // Apply PBR params
        meshRender.UseCustomPBR = true;
        meshRender.Metallic = mat->metallic;
        meshRender.Roughness = mat->roughness;
        meshRender.Reflectance = mat->reflectance;
        meshRender.AO = mat->ao;
        meshRender.Color = mat->baseColor;
        meshRender.EmissiveColor = mat->emissiveColor;
        meshRender.EmissiveFactor = mat->emissiveFactor;
        
        // Create TextureMaterial with textures from paths
        auto texMat = std::make_shared<se::TextureMaterial>();
        texMat->BaseColor = mat->baseColor;
        texMat->MetallicFactor = mat->metallic;
        texMat->RoughnessFactor = mat->roughness;
        
        // Load textures using TextureManager (cached)
        if (mat->useAlbedoTexture && !mat->albedoTexturePath.empty()) {
            texMat->Albedo = se::TextureManager::Load(mat->albedoTexturePath);
        }
        if (mat->useNormalTexture && !mat->normalTexturePath.empty()) {
            texMat->Normal = se::TextureManager::Load(mat->normalTexturePath);
        }
        if (mat->useMetallicTexture && !mat->metallicTexturePath.empty()) {
            texMat->Metallic = se::TextureManager::Load(mat->metallicTexturePath);
        }
        if (mat->useRoughnessTexture && !mat->roughnessTexturePath.empty()) {
            texMat->Roughness = se::TextureManager::Load(mat->roughnessTexturePath);
        }
        if (mat->useAOTexture && !mat->aoTexturePath.empty()) {
            texMat->AO = se::TextureManager::Load(mat->aoTexturePath);
        }
        if (mat->useEmissiveTexture && !mat->emissiveTexturePath.empty()) {
            texMat->Emissive = se::TextureManager::Load(mat->emissiveTexturePath);
        }
        
        meshRender.customTextureMaterial = texMat;
        appliedCount++;
    }
    
    SE_LOG_INFO("EditorContext: Applied materials to {} entities", appliedCount);
}

bool EditorContext::SaveMaterialToPath(const std::string& materialName, const std::string& absolutePath) {
    EditorMaterialData* mat = GetMaterialByName(materialName);
    if (!mat) {
        SE_LOG_ERROR("EditorContext: Material '{}' not found, cannot save to '{}'", materialName, absolutePath);
        return false;
    }
    
    if (MaterialSerializer::Save(*mat, absolutePath)) {
        SE_LOG_INFO("EditorContext: Saved compiled material '{}' to '{}'", materialName, absolutePath);
        return true;
    }
    
    SE_LOG_ERROR("EditorContext: Failed to save compiled material '{}' to '{}'", materialName, absolutePath);
    return false;
}

bool EditorContext::LoadCompiledMaterial(const std::string& absolutePath) {
    EditorMaterialData mat;
    if (!MaterialSerializer::Load(mat, absolutePath)) {
        SE_LOG_ERROR("EditorContext: Failed to load compiled material from '{}'", absolutePath);
        return false;
    }
    
    // Check if material with same name already exists
    EditorMaterialData* existing = GetMaterialByName(mat.name);
    if (existing) {
        SE_LOG_INFO("EditorContext: Updating existing material '{}' from compiled binary", mat.name);
        *existing = std::move(mat);
        existing->filePath = absolutePath;
        existing->isDirty = false;
    } else {
        mat.filePath = absolutePath;
        mat.isDirty = false;
        materials_.push_back(std::move(mat));
        SE_LOG_INFO("EditorContext: Loaded compiled material '{}' from '{}'", 
                    materials_.back().name, absolutePath);
    }
    
    return true;
}

bool EditorContext::SaveMapWithCompiledMaterials(const std::string& mapPath) {
    SE_LOG_INFO("EditorContext: Saving map with compiled materials to '{}'", mapPath);
    
    // Determine compiled materials directory based on map path
    std::filesystem::path path(mapPath);
    std::filesystem::path assetsPath = path.parent_path().parent_path();
    std::filesystem::path compiledDir = assetsPath / "compiled_materials" / path.stem();
    
    // Create compiled_materials directory
    std::error_code ec;
    std::filesystem::create_directories(compiledDir, ec);
    if (ec) {
        SE_LOG_WARN("EditorContext: Failed to create compiled_materials directory: {}", ec.message());
    } else {
        SE_LOG_INFO("EditorContext: Created compiled materials directory at '{}'", compiledDir.string());
    }
    
    // Find all entities with custom materials and save their materials
    auto& scene = sceneManager_->GetScene();
    auto view = scene.GetAllEntitiesWith<se::MeshRenderComponent, PrimitiveFactory::EditorMetadata>();
    
    std::set<std::string> savedMaterials;
    
    for (auto entityHandle : view) {
        se::Entity entity(entityHandle, sceneManager_->GetScenePtr());
        auto& metadata = entity.GetComponent<PrimitiveFactory::EditorMetadata>();
        
        if (!metadata.hasCustomMaterial || metadata.materialName.empty()) {
            continue;
        }
        
        // Only save each material once
        if (savedMaterials.contains(metadata.materialName)) {
            continue;
        }
        
        // Sanitize material name for filename
        std::string sanitizedName = metadata.materialName;
        std::replace(sanitizedName.begin(), sanitizedName.end(), ' ', '_');
        
        std::filesystem::path materialFilePath = compiledDir / (sanitizedName + ".mstmat");
        std::string relativePath = "compiled_materials/" + path.stem().string() + "/" + sanitizedName + ".mstmat";
        
        // Save the material binary with embedded textures
        if (SaveMaterialToPath(metadata.materialName, materialFilePath.string())) {
            // Update the entity's compiled material path
            metadata.compiledMaterialPath = relativePath;
            savedMaterials.insert(metadata.materialName);
            SE_LOG_INFO("EditorContext: Compiled material '{}' to '{}'", metadata.materialName, relativePath);
        } else {
            SE_LOG_ERROR("EditorContext: Failed to compile material '{}' to '{}'", 
                         metadata.materialName, materialFilePath.string());
        }
    }
    
    SE_LOG_INFO("EditorContext: Compiled {} materials for map '{}'", 
                savedMaterials.size(), path.stem().string());
    
    // Now save the map (which will include the updated compiledMaterialPath values)
    return document_->SaveAs(mapPath);
}

void EditorContext::LoadCompiledMaterialsFromMap(const std::string& assetsBasePath) {
    SE_LOG_INFO("EditorContext: Loading compiled materials from base path '{}'", assetsBasePath);
    
    auto& scene = sceneManager_->GetScene();
    auto view = scene.GetAllEntitiesWith<se::MeshRenderComponent, PrimitiveFactory::EditorMetadata>();
    
    std::set<std::string> loadedMaterials;
    int loadCount = 0;
    
    for (auto entityHandle : view) {
        se::Entity entity(entityHandle, sceneManager_->GetScenePtr());
        auto& metadata = entity.GetComponent<PrimitiveFactory::EditorMetadata>();
        
        if (!metadata.hasCustomMaterial || metadata.compiledMaterialPath.empty()) {
            continue;
        }
        
        // Skip if already loaded
        if (loadedMaterials.contains(metadata.compiledMaterialPath)) {
            continue;
        }
        
        // Build absolute path from assets base
        std::filesystem::path absolutePath = std::filesystem::path(assetsBasePath) / metadata.compiledMaterialPath;
        
        if (std::filesystem::exists(absolutePath)) {
            if (LoadCompiledMaterial(absolutePath.string())) {
                loadedMaterials.insert(metadata.compiledMaterialPath);
                loadCount++;
            }
        } else {
            SE_LOG_WARN("EditorContext: Compiled material not found: '{}'", absolutePath.string());
        }
    }
    
    SE_LOG_INFO("EditorContext: Loaded {} compiled materials from map", loadCount);
    
    // Now apply the loaded materials to entities
    ApplyMaterialsToLoadedEntities();
}

}  // namespace mst

