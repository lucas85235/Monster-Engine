#include "core/PrimitiveFactory.h"

#include <filesystem>

#include "engine/Application.h"
#include "engine/Log.h"
#include "engine/ecs/FilamentComponents.h"
#include "engine/renderer/MaterialSystem.h"
#include "engine/renderer/MeshData.h"
#include "engine/renderer/MeshSystem.h"

namespace mst {

namespace fs = std::filesystem;

uint32_t PrimitiveFactory::primitiveCounter_ = 0;
bool PrimitiveFactory::materialInitialized_ = false;
se::MaterialHandle PrimitiveFactory::cachedMaterial_{};

se::Entity PrimitiveFactory::CreatePrimitive(se::Scene& scene, PrimitiveType type,
                                              const std::string& name) {
    std::string entityName = name;
    if (entityName.empty()) {
        entityName = std::string(PrimitiveTypeToString(type)) + "_" +
                     std::to_string(primitiveCounter_++);
    }

    se::Entity entity = scene.CreateEntity(entityName);

    auto meshData = GetPrimitiveMeshData(type);
    auto material = GetDefaultMaterial();

    if (!meshData.IsValid()) {
        SE_LOG_ERROR("PrimitiveFactory: Failed to get mesh data for type {}", PrimitiveTypeToString(type));
    }
    if (!material.IsValid()) {
        SE_LOG_ERROR("PrimitiveFactory: Failed to get default material");
    }

    // Create Filament renderable via MeshSystem
    auto& meshSystem = se::Application::Get().GetMeshSystem();
    if (meshData.IsValid() && material.IsValid()) {
        auto renderableHandle = meshSystem.CreateRenderable(meshData, material);
        entity.AddComponent<se::FilamentRenderableComponent>(renderableHandle, material);
    }

    // Keep MeshRenderComponent for legacy data storage (colors, PBR params).
    // The vertex_array and material pointers remain null — Filament handles rendering.
    entity.AddComponent<se::MeshRenderComponent>();

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

se::MeshData PrimitiveFactory::GetPrimitiveMeshData(PrimitiveType type) {
    switch (type) {
        case PrimitiveType::Cube:
            return se::MeshPrimitives::CreateBox();
        case PrimitiveType::Sphere:
            return se::MeshPrimitives::CreateSphere();
        case PrimitiveType::Capsule:
            // No capsule primitive yet; approximate with a stretched sphere
            return se::MeshPrimitives::CreateSphere(0.5f, 32, 16);
        case PrimitiveType::Cylinder:
            return se::MeshPrimitives::CreateCylinder();
        case PrimitiveType::Plane:
            return se::MeshPrimitives::CreatePlane();
        default:
            return se::MeshPrimitives::CreateBox();
    }
}

se::MaterialHandle PrimitiveFactory::GetDefaultMaterial() {
    // Return cached material if available
    if (materialInitialized_ && cachedMaterial_.IsValid()) {
        return cachedMaterial_;
    }

    auto& materialSystem = se::Application::Get().GetMaterialSystem();

    cachedMaterial_ = materialSystem.GetDefaultLit();
    materialInitialized_ = true;
    SE_LOG_INFO("PrimitiveFactory: Using Filament default lit material");

    return cachedMaterial_;
}

}  // namespace mst
