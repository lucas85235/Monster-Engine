#include "engine/ecs/Scene.h"

#include "engine/core/Application.h"
#include "engine/core/Log.h"
#include "engine/ecs/RenderSystem.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/physics/PhysicsSystem.h"

namespace se {

Scene::Scene(const std::string& name, const SceneSettings& settings) : name_(name) {
    SE_LOG_INFO("Scene '{}' created", name_);

    if (settings.EnablePhysics) {
        physics_system_ = std::make_unique<PhysicsSystem>(this);
        physics_system_->Initialize();
        SE_LOG_INFO("Scene '{}' physics enabled", name_);
    }
}

Scene::~Scene() {
    Clear();

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

    // SE_LOG_INFO("Entity '{}' created with ID: {}", name, entity.GetID());

    return entity;
}

void Scene::DestroyEntity(Entity entity) {
    if (!entity.IsValid()) {
        SE_LOG_WARN("Attempted to destroy invalid entity");
        return;
    }

    auto& nameComp = entity.GetComponent<NameComponent>();
    // SE_LOG_INFO("Entity '{}' destroyed", nameComp.Name);

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
    if (physics_system_) { physics_system_->Update(deltaTime); }
}

void Scene::OnRender(const Camera& camera, float aspectRatio) {
    RenderSystem::Render(*this, camera, aspectRatio);

    if (physics_system_) { physics_system_->RenderDebug(camera); }
}

void Scene::Clear() {
    SE_LOG_INFO("Clearing scene '{}'", name_);
    registry_.clear();
}

}  // namespace se
