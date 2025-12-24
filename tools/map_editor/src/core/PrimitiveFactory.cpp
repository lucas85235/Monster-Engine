#include "core/PrimitiveFactory.h"

#include "engine/Log.h"
#include "engine/resources/MeshManager.h"

namespace mst {

uint32_t PrimitiveFactory::primitiveCounter_ = 0;

se::Entity PrimitiveFactory::CreatePrimitive(se::Scene& scene, PrimitiveType type,
                                              const std::string& name) {
    std::string entityName = name;
    if (entityName.empty()) {
        entityName = std::string(PrimitiveTypeToString(type)) + "_" +
                     std::to_string(primitiveCounter_++);
    }

    se::Entity entity = scene.CreateEntity(entityName);

    auto mesh = GetPrimitiveMesh(type);
    entity.AddComponent<se::MeshRenderComponent>(mesh, nullptr);

    EditorMetadata metadata;
    metadata.primitiveType = type;

    switch (type) {
        case PrimitiveType::Cube:
            metadata.colliderType = ColliderType::Box;
            metadata.colliderSize = {1.0f, 1.0f, 1.0f};
            break;
        case PrimitiveType::Sphere:
            metadata.colliderType   = ColliderType::Sphere;
            metadata.colliderRadius = 0.5f;
            break;
        case PrimitiveType::Capsule:
            metadata.colliderType   = ColliderType::Capsule;
            metadata.colliderRadius = 0.5f;
            metadata.colliderHeight = 1.0f;
            break;
        case PrimitiveType::Cylinder:
            metadata.colliderType = ColliderType::Box;
            metadata.colliderSize = {1.0f, 1.0f, 1.0f};
            break;
        case PrimitiveType::Plane:
            metadata.colliderType = ColliderType::Box;
            metadata.colliderSize = {10.0f, 0.1f, 10.0f};
            break;
    }

    entity.AddComponent<EditorMetadata>(metadata);

    SE_LOG_INFO("PrimitiveFactory: Created {} '{}'", PrimitiveTypeToString(type), entityName);

    return entity;
}

std::shared_ptr<se::VertexArray> PrimitiveFactory::GetPrimitiveMesh(PrimitiveType type) {
    switch (type) {
        case PrimitiveType::Cube: 
            return se::MeshManager::GetPrimitive(se::PrimitiveMeshType::Cube);
        case PrimitiveType::Sphere:
            return se::MeshManager::GetPrimitive(se::PrimitiveMeshType::Sphere);
        case PrimitiveType::Capsule:
            return se::MeshManager::GetPrimitive(se::PrimitiveMeshType::Capsule);
        case PrimitiveType::Cylinder:
            return se::MeshManager::GetPrimitive(se::PrimitiveMeshType::Cylinder);
        case PrimitiveType::Plane: 
            return se::MeshManager::GetPrimitive(se::PrimitiveMeshType::Quad);
        default: 
            return se::MeshManager::GetPrimitive(se::PrimitiveMeshType::Cube);
    }
}

std::shared_ptr<se::Material> PrimitiveFactory::GetDefaultMaterial() {
    return nullptr;
}

}  // namespace mst
