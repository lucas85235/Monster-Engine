#include "SceneManager.h"

#include "EventBus.h"
#include "engine/Application.h"
#include "engine/Log.h"
#include "engine/ecs/SimpleComponents.h"

using namespace se;

namespace mst {

SceneManager::SceneManager(EventBus& eventBus) : eventBus_(eventBus) {
    CreateNewScene();
}

void SceneManager::CreateNewScene(const std::string& name) {
    scene_ = se::CreateScope<se::Scene>(name, se::SceneSettings{.EnablePhysics = false});
    se::Application::Get().SetActiveScene(scene_.get());
    
    CreateDefaultLight();
    
    SE_LOG_INFO("SceneManager: Created new scene '{}'", name);
    eventBus_.Publish(MapClearedEvent{});
}

void SceneManager::ClearScene() {
    if (!scene_) return;
    
    scene_->Clear();
    CreateDefaultLight();
    
    SE_LOG_INFO("SceneManager: Scene cleared");
    eventBus_.Publish(MapClearedEvent{});
}

void SceneManager::CreateDefaultLight() {
    editorLight_ = scene_->CreateEntity("Editor Light");
    
    auto& transform = editorLight_.GetComponent<se::TransformComponent>();
    transform.SetPosition({0.0f, 10.0f, 10.0f});
    transform.SetRotation({-45.0f, 0.0f, 0.0f});
    
    auto& light = editorLight_.AddComponent<se::DirectionalLightComponent>();
    light.Color = {1.0f, 0.98f, 0.9f};
    light.Intensity = 1.5f;
    light.CastShadows = true;
    
    SE_LOG_INFO("SceneManager: Editor light created");
}

}  // namespace mst
