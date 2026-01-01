#pragma once
/**
 * EditorContext.h - Central state container for editor subsystems.
 *
 * Provides a single point of access to all editor components following
 * the Mediator pattern. Components access shared state through this context.
 */

#include <memory>
#include <vector>

#include "Engine.h"
#include "EditorMaterialData.h"
#include "EntityManager.h"
#include "EventBus.h"
#include "MapDocument.h"
#include "SceneManager.h"
#include "../commands/CommandSystem.h"
#include "../editor/EditorCamera.h"
#include "../editor/GizmoController.h"
#include "../editor/SelectionManager.h"

namespace mst {

class FileDialogManager;

class EditorContext {
public:
    EditorContext();
    ~EditorContext();
    
    // Core systems
    EventBus& GetEventBus() { return eventBus_; }
    CommandSystem& GetCommandSystem() { return *commandSystem_; }
    
    // Scene management
    SceneManager& GetSceneManager() { return *sceneManager_; }
    EntityManager& GetEntityManager() { return *entityManager_; }
    MapDocument& GetDocument() { return *document_; }
    
    // Save map with compiled materials (handles material binary generation)
    bool SaveMapWithCompiledMaterials(const std::string& mapPath);
    
    // Editor state
    SelectionManager& GetSelection() { return selection_; }
    GizmoController& GetGizmo() { return gizmo_; }
    EditorCamera& GetCamera() { return camera_; }
    
    // Convenience accessors
    se::Scene& GetScene() { return sceneManager_->GetScene(); }
    
    // File dialogs
    void SetFileDialogManager(FileDialogManager* fdm) { fileDialogs_ = fdm; }
    FileDialogManager* GetFileDialogs() { return fileDialogs_; }
    
    // Material management
    size_t GetMaterialCount() const { return materials_.size(); }
    EditorMaterialData* GetMaterialByIndex(int index);
    EditorMaterialData* GetMaterialByName(const std::string& name);
    int GetMaterialIndex(const std::string& name) const;
    
    void CreateNewMaterial();
    void DuplicateMaterial(int index);
    void RemoveMaterial(int index);
    bool LoadMaterial(const std::string& path);
    bool SaveMaterial(int index, const std::string& path);
    bool SaveMaterialToPath(const std::string& materialName, const std::string& absolutePath);
    void ClearMaterials();
    
    const std::vector<EditorMaterialData>& GetMaterials() const { return materials_; }
    
    // Apply materials to all entities that have custom materials assigned
    // Call this after loading a map to recreate TextureMaterial on entities
    void ApplyMaterialsToLoadedEntities();
    
    // Load a compiled material from binary path
    bool LoadCompiledMaterial(const std::string& absolutePath);
    
    // Load compiled materials from a map file (uses entity compiledMaterialPath)
    void LoadCompiledMaterialsFromMap(const std::string& assetsBasePath);

private:
    EventBus eventBus_;
    se::Scope<CommandSystem> commandSystem_;
    se::Scope<SceneManager> sceneManager_;
    se::Scope<EntityManager> entityManager_;
    se::Scope<MapDocument> document_;
    
    SelectionManager selection_;
    GizmoController gizmo_;
    EditorCamera camera_;
    
    // File dialogs reference
    FileDialogManager* fileDialogs_ = nullptr;
    
    // Material library
    std::vector<EditorMaterialData> materials_;
    int nextMaterialId_ = 1;
};

}  // namespace mst

