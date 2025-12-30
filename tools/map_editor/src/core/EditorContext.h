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
    
    // Editor state
    SelectionManager& GetSelection() { return selection_; }
    GizmoController& GetGizmo() { return gizmo_; }
    EditorCamera& GetCamera() { return camera_; }
    
    // Convenience accessors
    se::Scene& GetScene() { return sceneManager_->GetScene(); }
    
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
    void ClearMaterials();
    
    const std::vector<EditorMaterialData>& GetMaterials() const { return materials_; }

private:
    EventBus eventBus_;
    se::Scope<CommandSystem> commandSystem_;
    se::Scope<SceneManager> sceneManager_;
    se::Scope<EntityManager> entityManager_;
    se::Scope<MapDocument> document_;
    
    SelectionManager selection_;
    GizmoController gizmo_;
    EditorCamera camera_;
    
    // Material library
    std::vector<EditorMaterialData> materials_;
    int nextMaterialId_ = 1;
};

}  // namespace mst

