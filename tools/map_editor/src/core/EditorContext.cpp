#include "EditorContext.h"

#include "MaterialSerializer.h"
#include "engine/Log.h"

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

}  // namespace mst

