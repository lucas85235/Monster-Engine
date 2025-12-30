#include "EntityManager.h"

#include "EventBus.h"
#include "SceneManager.h"
#include "engine/Log.h"
#include "engine/ecs/SimpleComponents.h"

using namespace se;

namespace mst {

EntityManager::EntityManager(SceneManager& sceneManager, EventBus& eventBus)
    : sceneManager_(sceneManager), eventBus_(eventBus) {}

se::Entity EntityManager::CreatePrimitive(PrimitiveType type, const std::string& name) {
    se::Entity entity = PrimitiveFactory::CreatePrimitive(sceneManager_.GetScene(), type, name);
    
    eventBus_.Publish(EntityCreatedEvent{entity});
    SE_LOG_INFO("EntityManager: Created primitive '{}'", 
                entity.GetComponent<se::NameComponent>().Name);
    
    return entity;
}

se::Entity EntityManager::CreateFromData(const MapEntityData& data) {
    se::Entity entity = PrimitiveFactory::CreatePrimitive(
        sceneManager_.GetScene(), data.primitiveType, data.name);
    
    auto& transform = entity.GetComponent<se::TransformComponent>();
    transform.SetPosition(data.position);
    transform.SetRotation(data.rotation);
    transform.SetScale(data.scale);
    
    if (entity.HasComponent<se::MeshRenderComponent>()) {
        auto& mesh = entity.GetComponent<se::MeshRenderComponent>();
        mesh.Color = data.color;
        mesh.EmissiveColor = data.emissiveColor;
        mesh.EmissiveFactor = data.emissiveFactor;
    }
    
    if (entity.HasComponent<PrimitiveFactory::EditorMetadata>()) {
        auto& metadata = entity.GetComponent<PrimitiveFactory::EditorMetadata>();
        metadata.hasCollision = data.hasCollision;
        metadata.colliderType = data.colliderType;
        metadata.colliderSize = data.colliderSize;
        metadata.colliderRadius = data.colliderRadius;
        metadata.colliderHeight = data.colliderHeight;
        metadata.rigidbodyType = data.rigidbodyType;
        metadata.mass = data.mass;
        
        // Material reference (version 5+)
        metadata.hasCustomMaterial = data.hasCustomMaterial;
        metadata.materialName = data.materialName;
    }
    
    eventBus_.Publish(EntityCreatedEvent{entity});
    return entity;
}

se::Entity EntityManager::DuplicateEntity(se::Entity source) {
    if (!source.IsValid()) return se::Entity();
    
    auto& name = source.GetComponent<se::NameComponent>().Name;
    auto& transform = source.GetComponent<se::TransformComponent>();
    auto& metadata = source.GetComponent<PrimitiveFactory::EditorMetadata>();
    
    se::Entity duplicate = PrimitiveFactory::CreatePrimitive(
        sceneManager_.GetScene(), metadata.primitiveType, name + "_copy");
    
    auto& dupTransform = duplicate.GetComponent<se::TransformComponent>();
    dupTransform.SetPosition(transform.Position + Vector3(1.0f, 0.0f, 0.0f));
    dupTransform.SetRotation(transform.Rotation);
    dupTransform.SetScale(transform.Scale);
    
    auto& dupMetadata = duplicate.GetComponent<PrimitiveFactory::EditorMetadata>();
    dupMetadata.hasCollision = metadata.hasCollision;
    dupMetadata.colliderType = metadata.colliderType;
    dupMetadata.colliderSize = metadata.colliderSize;
    dupMetadata.colliderRadius = metadata.colliderRadius;
    dupMetadata.colliderHeight = metadata.colliderHeight;
    dupMetadata.rigidbodyType = metadata.rigidbodyType;
    dupMetadata.mass = metadata.mass;
    
    if (source.HasComponent<se::MeshRenderComponent>()) {
        auto& srcMesh = source.GetComponent<se::MeshRenderComponent>();
        auto& dupMesh = duplicate.GetComponent<se::MeshRenderComponent>();
        dupMesh.Color = srcMesh.Color;
        dupMesh.EmissiveColor = srcMesh.EmissiveColor;
        dupMesh.EmissiveFactor = srcMesh.EmissiveFactor;
    }
    
    eventBus_.Publish(EntityCreatedEvent{duplicate});
    SE_LOG_INFO("EntityManager: Duplicated '{}' -> '{}'", name, name + "_copy");
    
    return duplicate;
}

void EntityManager::DeleteEntity(se::Entity entity) {
    if (!entity.IsValid()) return;
    
    auto& name = entity.GetComponent<se::NameComponent>().Name;
    uint32_t id = entity.GetID();
    
    if (entity == playerStart_) {
        playerStart_ = se::Entity();
    }
    
    sceneManager_.GetScene().DestroyEntity(entity);
    
    eventBus_.Publish(EntityDeletedEvent{id, name});
    SE_LOG_INFO("EntityManager: Deleted entity '{}'", name);
}

void EntityManager::DeleteEntities(const std::vector<se::Entity>& entities) {
    for (auto entity : entities) {
        DeleteEntity(entity);
    }
}

se::Entity EntityManager::CreatePlayerStart() {
    if (playerStart_.IsValid()) {
        SE_LOG_WARN("EntityManager: Player Start already exists!");
        return playerStart_;
    }
    
    playerStart_ = sceneManager_.GetScene().CreateEntity("Player Start");
    
    auto& transform = playerStart_.GetComponent<se::TransformComponent>();
    transform.SetPosition({0.0f, 0.0f, 0.0f});
    
    auto mesh = PrimitiveFactory::GetPrimitiveMesh(PrimitiveType::Capsule);
    auto material = PrimitiveFactory::GetDefaultMaterial();
    auto& meshRender = playerStart_.AddComponent<se::MeshRenderComponent>(mesh, material);
    meshRender.Color = {0.2f, 0.8f, 0.2f, 0.7f};
    
    eventBus_.Publish(EntityCreatedEvent{playerStart_});
    SE_LOG_INFO("EntityManager: Created Player Start");
    
    return playerStart_;
}

MapEntityData EntityManager::SerializeEntity(se::Entity entity) const {
    MapEntityData data;
    
    if (!entity.IsValid()) return data;
    
    data.name = entity.GetComponent<se::NameComponent>().Name;
    
    auto& transform = entity.GetComponent<se::TransformComponent>();
    data.position = transform.Position;
    data.rotation = transform.Rotation;
    data.scale = transform.Scale;
    
    if (entity.HasComponent<PrimitiveFactory::EditorMetadata>()) {
        auto& metadata = entity.GetComponent<PrimitiveFactory::EditorMetadata>();
        data.primitiveType = metadata.primitiveType;
        data.hasCollision = metadata.hasCollision;
        data.colliderType = metadata.colliderType;
        data.colliderSize = metadata.colliderSize;
        data.colliderRadius = metadata.colliderRadius;
        data.colliderHeight = metadata.colliderHeight;
        data.rigidbodyType = metadata.rigidbodyType;
        data.mass = metadata.mass;
        
        // Material reference (version 5+)
        data.hasCustomMaterial = metadata.hasCustomMaterial;
        data.materialName = metadata.materialName;
    }
    
    if (entity.HasComponent<se::MeshRenderComponent>()) {
        auto& mesh = entity.GetComponent<se::MeshRenderComponent>();
        data.color = mesh.Color;
        data.emissiveColor = mesh.EmissiveColor;
        data.emissiveFactor = mesh.EmissiveFactor;
    }
    
    return data;
}

}  // namespace mst
