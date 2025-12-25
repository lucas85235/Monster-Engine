#pragma once
/**
 * SceneManager.h - Scene lifecycle and default entity management.
 *
 * Handles scene creation, clearing, and ensures required scene entities
 * like the editor light are always present.
 */

#include <memory>
#include <string>

#include "Engine.h"
#include "engine/ecs/Scene.h"

namespace mst {

class EventBus;

class SceneManager {
public:
    explicit SceneManager(EventBus& eventBus);
    
    void CreateNewScene(const std::string& name = "Editor Scene");
    void ClearScene();
    
    se::Scene& GetScene() { return *scene_; }
    const se::Scene& GetScene() const { return *scene_; }
    se::Scene* GetScenePtr() { return scene_.get(); }
    
    se::Entity GetEditorLight() const { return editorLight_; }

private:
    void CreateDefaultLight();
    
    EventBus& eventBus_;
    se::Scope<se::Scene> scene_;
    se::Entity editorLight_;
};

}  // namespace mst
