#include "engine/resources/MapLoader.h"

#include <fstream>
#include <algorithm>

#include "engine/Log.h"
#include "engine/MeshFactory.h"
#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/resources/MaterialManager.h"
#include "engine/resources/MeshManager.h"
#include "engine/physics/Collider.h"
#include "engine/physics/RigidbodyComponent.h"
#include "engine/physics/PhysicsSystem.h"

namespace se {

MapLoadResult MapLoader::Load(Scene& scene, const std::filesystem::path& path) {
    MapLoadResult result;
    
    SE_LOG_INFO("MapLoader: Loading map from '{}'", path.string());
    
    std::ifstream file(path, std::ios::binary);
    if (!file.is_open()) {
        SE_LOG_ERROR("MapLoader: Failed to open file: {}", path.string());
        return result;
    }
    
    uint32_t entityCount = 0;
    uint32_t version = 0;
    if (!ReadHeader(file, entityCount, result, version)) {
        SE_LOG_ERROR("MapLoader: Failed to read header from: {}", path.string());
        return result;
    }
    
    // Read map name (skip it, not used at runtime)
    std::string mapName;
    if (!ReadString(file, mapName)) {
        SE_LOG_ERROR("MapLoader: Failed to read map name");
        return result;
    }
    
    // Read and create entities
    for (uint32_t i = 0; i < entityCount; ++i) {
        EntityData data;
        if (!ReadEntity(file, data)) {
            SE_LOG_ERROR("MapLoader: Failed to read entity {} from: {}", i, path.string());
            return result;
        }
        CreateSceneEntity(scene, data);
    }
    
    // Create directional light entity if map has one
    if (result.hasDirectionalLight) {
        CreateDirectionalLight(scene, result);
    }
    
    result.success = true;
    result.entityCount = entityCount;
    
    SE_LOG_INFO("MapLoader: Loaded {} entities, PlayerStart: {}, Light: {} (intensity: {})",
                entityCount,
                result.hasPlayerStart ? "Yes" : "No",
                result.hasDirectionalLight ? "Yes" : "No",
                result.lightIntensity);
    
    return result;
}

bool MapLoader::ReadString(std::ifstream& file, std::string& str) {
    uint32_t length = 0;
    file.read(reinterpret_cast<char*>(&length), sizeof(length));
    if (file.fail() || length > 10000) return false;
    
    str.resize(length);
    file.read(str.data(), length);
    return !file.fail();
}

bool MapLoader::ReadHeader(std::ifstream& file, uint32_t& entityCount, MapLoadResult& result, uint32_t& version) {
    uint32_t magic = 0;
    file.read(reinterpret_cast<char*>(&magic), sizeof(magic));
    if (magic != MAGIC) {
        SE_LOG_ERROR("MapLoader: Invalid file magic: expected 0x{:08X}, got 0x{:08X}", 
                     MAGIC, magic);
        return false;
    }
    
    file.read(reinterpret_cast<char*>(&version), sizeof(version));
    file.read(reinterpret_cast<char*>(&entityCount), sizeof(entityCount));
    
    // Version 2+ has Player Start data
    if (version >= 2) {
        uint8_t hasPlayerStart = 0;
        file.read(reinterpret_cast<char*>(&hasPlayerStart), sizeof(hasPlayerStart));
        result.hasPlayerStart = (hasPlayerStart != 0);
        file.read(reinterpret_cast<char*>(&result.playerStartPosition), sizeof(Vector3));
        file.read(reinterpret_cast<char*>(&result.playerStartRotation), sizeof(Vector3));
    } else {
        // Version 1 had a reserved field
        uint32_t reserved = 0;
        file.read(reinterpret_cast<char*>(&reserved), sizeof(reserved));
        result.hasPlayerStart = false;
    }
    
    // Version 3+ has Directional Light data
    if (version >= 3) {
        uint8_t hasLight = 0;
        file.read(reinterpret_cast<char*>(&hasLight), sizeof(hasLight));
        result.hasDirectionalLight = (hasLight != 0);
        if (result.hasDirectionalLight) {
            file.read(reinterpret_cast<char*>(&result.lightDirection), sizeof(Vector3));
            file.read(reinterpret_cast<char*>(&result.lightPosition), sizeof(Vector3));
            file.read(reinterpret_cast<char*>(&result.lightRotation), sizeof(Vector3));
            file.read(reinterpret_cast<char*>(&result.lightColor), sizeof(Vector3));
            file.read(reinterpret_cast<char*>(&result.lightIntensity), sizeof(float));
            uint8_t castShadows = 0;
            file.read(reinterpret_cast<char*>(&castShadows), sizeof(castShadows));
            result.lightCastShadows = (castShadows != 0);
            uint8_t enabled = 0;
            file.read(reinterpret_cast<char*>(&enabled), sizeof(enabled));
            result.lightEnabled = (enabled != 0);
        }
    } else {
        // Default light for older maps (version 1-2)
        result.hasDirectionalLight = true;
        result.lightDirection = Vector3(0.0f, -1.0f, 0.0f);
        result.lightPosition = Vector3(0.0f, 10.0f, 10.0f);
        result.lightRotation = Vector3(-45.0f, 0.0f, 0.0f);
        result.lightColor = Vector3(1.0f, 0.98f, 0.9f);
        result.lightIntensity = 1.5f;
        result.lightCastShadows = true;
        result.lightEnabled = true;
    }
    
    return !file.fail();
}

bool MapLoader::ReadEntity(std::ifstream& file, EntityData& entity) {
    if (!ReadString(file, entity.name)) return false;
    
    file.read(reinterpret_cast<char*>(&entity.primitiveType), sizeof(entity.primitiveType));
    file.read(reinterpret_cast<char*>(&entity.position), sizeof(Vector3));
    file.read(reinterpret_cast<char*>(&entity.rotation), sizeof(Vector3));
    file.read(reinterpret_cast<char*>(&entity.scale), sizeof(Vector3));
    file.read(reinterpret_cast<char*>(&entity.color), sizeof(Vector4));
    
    uint8_t hasCollision = 0;
    file.read(reinterpret_cast<char*>(&hasCollision), sizeof(hasCollision));
    entity.hasCollision = (hasCollision != 0);
    
    if (entity.hasCollision) {
        file.read(reinterpret_cast<char*>(&entity.colliderType), sizeof(entity.colliderType));
        file.read(reinterpret_cast<char*>(&entity.colliderSize), sizeof(Vector3));
        file.read(reinterpret_cast<char*>(&entity.colliderRadius), sizeof(float));
        file.read(reinterpret_cast<char*>(&entity.colliderHeight), sizeof(float));
        
        // Read rigidbody settings
        file.read(reinterpret_cast<char*>(&entity.rigidbodyType), sizeof(entity.rigidbodyType));
        file.read(reinterpret_cast<char*>(&entity.mass), sizeof(float));
    }
    
    return !file.fail();
}

void MapLoader::CreateSceneEntity(Scene& scene, const EntityData& data) {
    Entity entity = scene.CreateEntity(data.name);
    
    // Set transform
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.SetPosition(data.position);
    transform.SetRotation(data.rotation);
    transform.SetScale(data.scale);
    
    // Get mesh based on primitive type
    PrimitiveMeshType meshType;
    switch (data.primitiveType) {
        case PrimitiveType::Cube:     meshType = PrimitiveMeshType::Cube; break;
        case PrimitiveType::Sphere:   meshType = PrimitiveMeshType::Sphere; break;
        case PrimitiveType::Capsule:  meshType = PrimitiveMeshType::Capsule; break;
        case PrimitiveType::Cylinder: meshType = PrimitiveMeshType::Cylinder; break;
        case PrimitiveType::Plane:    meshType = PrimitiveMeshType::Quad; break;
        default:                      meshType = PrimitiveMeshType::Cube; break;
    }
    
    auto mesh = MeshManager::GetPrimitive(meshType);
    auto material = MaterialManager::GetDefaultMaterial();
    
    // Add mesh render component
    auto& meshRender = entity.AddComponent<MeshRenderComponent>(mesh, material);
    meshRender.Color = data.color;
    
    // Add collider component if has collision
    // NOTE: Do NOT multiply by scale here - PhysicsSystem applies transform.Scale automatically
    if (data.hasCollision) {
        switch (data.colliderType) {
            case ColliderType::Box: {
                auto& collider = entity.AddComponent<BoxCollider>();
                collider.Size = data.colliderSize;
                break;
            }
            case ColliderType::Sphere: {
                auto& collider = entity.AddComponent<SphereCollider>();
                collider.Radius = data.colliderRadius;
                break;
            }
            case ColliderType::Capsule: {
                auto& collider = entity.AddComponent<CapsuleCollider>();
                collider.Radius = data.colliderRadius;
                collider.Height = data.colliderHeight;
                break;
            }
            default:
                break;
        }
        
        // Add rigidbody with settings from map
        RigidbodyData rbData;
        switch (data.rigidbodyType) {
            case 0: rbData.type = RigidbodyType::Static; break;
            case 1: rbData.type = RigidbodyType::Dynamic; break;
            case 2: rbData.type = RigidbodyType::Kinematic; break;
            default: rbData.type = RigidbodyType::Static; break;
        }
        rbData.mass = (rbData.type == RigidbodyType::Dynamic) ? data.mass : 0.0f;
        
        entity.AddComponent<RigidbodyComponent>(rbData);
    }
}

void MapLoader::CreateDirectionalLight(Scene& scene, const MapLoadResult& result) {
    if (!result.hasDirectionalLight || !result.lightEnabled) {
        return;
    }
    
    Entity lightEntity = scene.CreateEntity("Map Light");
    
    auto& transform = lightEntity.GetComponent<TransformComponent>();
    transform.SetPosition(result.lightPosition);
    transform.SetRotation(result.lightRotation);
    
    auto& light = lightEntity.AddComponent<DirectionalLightComponent>();
    light.Color = result.lightColor;
    light.Intensity = result.lightIntensity;
    light.CastShadows = result.lightCastShadows;
    light.Enabled = result.lightEnabled;
    
    SE_LOG_INFO("MapLoader: Created directional light with intensity {} and color ({}, {}, {})",
                result.lightIntensity, result.lightColor.x, result.lightColor.y, result.lightColor.z);
}

}  // namespace se

