#include "core/PrimitiveFactory.h"

#include <filesystem>

#include "engine/Log.h"
#include "engine/resources/MaterialManager.h"
#include "engine/resources/MeshManager.h"

namespace mst {

namespace fs = std::filesystem;

uint32_t PrimitiveFactory::primitiveCounter_ = 0;
std::shared_ptr<se::Material> PrimitiveFactory::cachedMaterial_ = nullptr;

se::Entity PrimitiveFactory::CreatePrimitive(se::Scene& scene, PrimitiveType type,
                                              const std::string& name) {
    std::string entityName = name;
    if (entityName.empty()) {
        entityName = std::string(PrimitiveTypeToString(type)) + "_" +
                     std::to_string(primitiveCounter_++);
    }

    se::Entity entity = scene.CreateEntity(entityName);

    auto mesh = GetPrimitiveMesh(type);
    auto material = GetDefaultMaterial();
    
    if (!mesh) {
        SE_LOG_ERROR("PrimitiveFactory: Failed to get mesh for type {}", PrimitiveTypeToString(type));
    }
    if (!material) {
        SE_LOG_ERROR("PrimitiveFactory: Failed to get default material");
    }
    
    entity.AddComponent<se::MeshRenderComponent>(mesh, material);

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
    // Return cached material if available
    if (cachedMaterial_) {
        return cachedMaterial_;
    }
    
    // Try to load the instanced shader from assets (core shaders)
    fs::path assetsPath = fs::current_path() / "assets";
    if (!fs::exists(assetsPath)) {
        SE_LOG_WARN("PrimitiveFactory: Assets folder not found, using engine default material");
        return se::MaterialManager::GetDefaultMaterial();
    }
    
    fs::path vertPath = assetsPath / "shaders" / "core" / "instanced.vert";
    fs::path fragPath = assetsPath / "shaders" / "core" / "instanced.frag";
    
    if (!fs::exists(vertPath) || !fs::exists(fragPath)) {
        SE_LOG_WARN("PrimitiveFactory: Instanced shaders not found at {}, using engine default material", vertPath.string());
        return se::MaterialManager::GetDefaultMaterial();
    }
    
    auto shader = se::MaterialManager::GetShader("EditorInstancedShader", vertPath, fragPath);
    if (!shader) {
        SE_LOG_ERROR("PrimitiveFactory: Failed to load instanced shader");
        return se::MaterialManager::GetDefaultMaterial();
    }
    
    cachedMaterial_ = se::MaterialManager::CreateMaterial(shader);
    cachedMaterial_->SetFloat("uReflectance", 0.5f);
    SE_LOG_INFO("PrimitiveFactory: Created material with instanced shader");
    
    return cachedMaterial_;
}

}  // namespace mst


