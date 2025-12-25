#pragma once
/**
 * EditorContext.h - Central state container for editor subsystems.
 *
 * Provides a single point of access to all editor components following
 * the Mediator pattern. Components access shared state through this context.
 */

#include <memory>

#include "Engine.h"
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

private:
    EventBus eventBus_;
    se::Scope<CommandSystem> commandSystem_;
    se::Scope<SceneManager> sceneManager_;
    se::Scope<EntityManager> entityManager_;
    se::Scope<MapDocument> document_;
    
    SelectionManager selection_;
    GizmoController gizmo_;
    EditorCamera camera_;
};

}  // namespace mst
