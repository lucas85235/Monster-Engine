#include "engine/ecs/Scene.h"

#include "engine/Application.h"
#include "engine/Camera.h"
#include "engine/Log.h"
#include "engine/ecs/ComponentSystem.h"
#include "engine/ecs/RenderSystem.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/physics/PhysicsSystem.h"

namespace se {

Scene::Scene(const std::string& name, const SceneSettings& settings) : name_(name) {
    SE_LOG_INFO("Scene '{}' created", name_);

    // Initialize ComponentSystem (always enabled)
    component_system_ = std::make_unique<ComponentSystem>(this);
    SE_LOG_INFO("Scene '{}' component system enabled", name_);

    if (settings.EnablePhysics) {
        physics_system_ = std::make_unique<PhysicsSystem>(this);
        physics_system_->Initialize();
        SE_LOG_INFO("Scene '{}' physics enabled", name_);
    }
}

Scene::~Scene() {
    Clear();

    // Shutdown ComponentSystem first (components may reference physics)
    if (component_system_) {
        component_system_.reset();
        SE_LOG_INFO("Scene '{}' component system shutdown", name_);
    }

    if (physics_system_) {
        physics_system_->Shutdown();
        SE_LOG_INFO("Scene '{}' physics shutdown", name_);
    }

    SE_LOG_INFO("Scene '{}' destroyed", name_);
}

Entity Scene::CreateEntity(const std::string& name) {
    Entity entity(registry_.create(), this);

    entity.AddComponent<TransformComponent>();
    entity.AddComponent<NameComponent>(name.empty() ? "Entity" : name);

    SE_LOG_INFO("Entity '{}' created with ID: {}", name, entity.GetID());

    return entity;
}

void Scene::DestroyEntity(Entity entity) {
    if (!entity.IsValid()) {
        SE_LOG_WARN("Attempted to destroy invalid entity");
        return;
    }

    auto& nameComp = entity.GetComponent<NameComponent>();
    SE_LOG_INFO("Entity '{}' destroyed", nameComp.Name);

    registry_.destroy(entity.GetHandle());
}

Entity Scene::FindEntityByName(const std::string& name) {
    auto view = registry_.view<NameComponent>();

    for (auto entity : view) {
        auto& nameComp = view.get<NameComponent>(entity);
        if (nameComp.Name == name) { return Entity(entity, this); }
    }

    SE_LOG_WARN("Entity with name '{}' not found", name);
    return Entity();
}

void Scene::OnUpdate(float deltaTime) {
    // Physics simulation
    if (physics_system_) { physics_system_->Update(deltaTime); }

    // Component lifecycle
    if (component_system_) {
        component_system_->ProcessPendingStarts();
        component_system_->FixedUpdate(deltaTime);
        component_system_->Update(deltaTime);
        component_system_->LateUpdate(deltaTime);
    }
}

void Scene::OnRender(const Camera& camera, float aspectRatio) {
    RenderSystem::Render(*this, camera, aspectRatio);

    if (physics_system_) { physics_system_->RenderDebug(camera); }
}

void Scene::OnRender() {
    if (!active_camera_) {
        SE_LOG_WARN("Scene::OnRender() called but no active camera set!");
        return;
    }

    auto& window      = Application::Get().GetWindow();
    float aspectRatio = static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight());

    OnRender(*active_camera_, aspectRatio);
}

void Scene::Clear() {
    SE_LOG_INFO("Clearing scene '{}'", name_);
    registry_.clear();
}

}  // namespace se
