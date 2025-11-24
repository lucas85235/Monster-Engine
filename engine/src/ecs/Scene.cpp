#include "engine/ecs/Scene.h"

#include "engine/Log.h"
#include "engine/ecs/Components.h"
#include "engine/ecs/RenderSystem.h"

namespace se {

Scene::Scene(const std::string& name) : name_(name) {
    SE_LOG_INFO("Scene '{}' created", name_);
}

Scene::~Scene() {
    Clear();
    SE_LOG_INFO("Scene '{}' destroyed", name_);
}

Entity Scene::CreateEntity(const std::string& name) {
    Entity entity(registry_.create(), this);

    // All entities have Transform and Name components by default
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
    return Entity();  // Return invalid entity
}

void Scene::OnUpdate(float deltaTime) {
    // Systems can be implemented here
    // Example: Physics system, Animation system, etc.

    // For now, just a placeholder
    (void)deltaTime;
}

void Scene::OnRender(const Camera& camera, float aspectRatio) {
    RenderSystem::Render(*this, camera, aspectRatio);
}

void Scene::Clear() {
    SE_LOG_INFO("Clearing scene '{}'", name_);
    registry_.clear();
}

}  // namespace se