#pragma once
#include "Engine.h"
#include "engine/Log.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/ecs/Scene.h"
#include "engine/renderer/Material.h"
#include "engine/resources/MaterialManager.h"
#include "engine/resources/MeshManager.h"

namespace Utilities {
using namespace se;

using EngineMaterial = Ref<Material>;

inline Entity CreateCubeEntity(const std::string& name, const Vector3& position, const Vector3& scale, Scene* scene, const Ref<Material>& material) {
    SE_LOG_INFO("Creating cube entity: {}", name);

    auto entity = scene->CreateEntity(name);

    Ref vertex_array = MeshManager::GetPrimitive(PrimitiveMeshType::Cube);

    if (!vertex_array) {
        SE_LOG_ERROR("Failed to get cube mesh!");
        return {};
    }

    entity.AddComponent<MeshRenderComponent>(vertex_array, CreateRef<Material>(*material));

    // Set position
    TransformComponent& transform = entity.GetComponent<TransformComponent>();
    transform.SetPosition(position);
    transform.SetScale(scale);

    SE_LOG_INFO("Cube entity created successfully at ({}, {}, {})", position.x, position.y, position.z);

    return entity;
}

inline EngineMaterial LoadMaterial() {
    auto assets_folder = findAssetsFolder();

    fs::path fragment_shader_location = assets_folder.value() / "shaders" / "basic.frag";
    fs::path vertex_shader_location   = assets_folder.value() / "shaders" / "basic.vert";

    Ref shader = MaterialManager::GetShader("DefaultShader", vertex_shader_location, fragment_shader_location);

    EngineMaterial material = MaterialManager::CreateMaterial(shader);
    material->SetFloat("uSpecularStrength", 0.5f);

    return material;
}
}  // namespace Utilities