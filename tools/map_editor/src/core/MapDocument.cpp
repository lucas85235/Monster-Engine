#include "MapDocument.h"

#include <algorithm>
#include <filesystem>

#include "EntityManager.h"
#include "EventBus.h"
#include "MapSerializer.h"
#include "PrimitiveFactory.h"
#include "SceneManager.h"
#include "engine/Log.h"
#include "engine/ecs/SimpleComponents.h"

using namespace se;

namespace mst {

MapDocument::MapDocument(EntityManager& entityManager, SceneManager& sceneManager, EventBus& eventBus)
    : entityManager_(entityManager), sceneManager_(sceneManager), eventBus_(eventBus) {
    mapData_.mapName = "Untitled Map";
}

void MapDocument::New() {
    sceneManager_.ClearScene();
    entityManager_.ClearPlayerStart();
    
    mapData_.Clear();
    mapData_.mapName = "Untitled Map";
    filePath_.clear();
    isDirty_ = false;
    
    SE_LOG_INFO("MapDocument: New map created");
    eventBus_.Publish(MapClearedEvent{});
}

bool MapDocument::Open(const std::string& path) {
    MapData loadedData;
    
    if (!MapSerializer::Import(loadedData, path)) {
        SE_LOG_ERROR("MapDocument: Failed to open '{}'", path);
        return false;
    }
    
    sceneManager_.ClearScene();
    entityManager_.ClearPlayerStart();
    
    LoadFromData(loadedData);
    
    mapData_ = std::move(loadedData);
    filePath_ = path;
    isDirty_ = false;
    
    SE_LOG_INFO("MapDocument: Opened '{}' with {} entities", path, mapData_.entities.size());
    eventBus_.Publish(MapLoadedEvent{path, mapData_.entities.size()});
    
    return true;
}

bool MapDocument::Save() {
    if (filePath_.empty()) {
        SE_LOG_WARN("MapDocument: No file path set, use SaveAs");
        return false;
    }
    return SaveAs(filePath_);
}

bool MapDocument::SaveAs(const std::string& path) {
    BuildMapData();
    
    // Create compiled materials directory based on map location
    std::filesystem::path mapPath(path);
    std::filesystem::path assetsPath = mapPath.parent_path().parent_path(); // Go up from maps/ to assets/
    std::filesystem::path compiledMaterialsPath = assetsPath / "compiled_materials" / mapPath.stem();
    
    // Ensure compiled_materials directory exists
    std::error_code ec;
    std::filesystem::create_directories(compiledMaterialsPath, ec);
    if (ec) {
        SE_LOG_WARN("MapDocument: Failed to create compiled_materials directory: {}", ec.message());
    } else {
        SE_LOG_INFO("MapDocument: Created compiled materials directory at '{}'", compiledMaterialsPath.string());
    }
    
    // Compile materials for entities that have custom materials
    for (auto& entityData : mapData_.entities) {
        if (entityData.hasCustomMaterial && !entityData.materialName.empty()) {
            // Generate compiled material filename
            std::string sanitizedName = entityData.materialName;
            std::replace(sanitizedName.begin(), sanitizedName.end(), ' ', '_');
            std::filesystem::path materialPath = compiledMaterialsPath / (sanitizedName + ".mstmat");
            
            // Store the relative path from assets folder
            entityData.compiledMaterialPath = "compiled_materials/" + mapPath.stem().string() + "/" + sanitizedName + ".mstmat";
            
            SE_LOG_INFO("MapDocument: Material '{}' will be saved to '{}'", 
                        entityData.materialName, entityData.compiledMaterialPath);
        }
    }
    
    if (!MapSerializer::Export(mapData_, path)) {
        SE_LOG_ERROR("MapDocument: Failed to save to '{}'", path);
        return false;
    }
    
    filePath_ = path;
    isDirty_ = false;
    
    SE_LOG_INFO("MapDocument: Saved to '{}'", path);
    eventBus_.Publish(MapSavedEvent{path});
    
    return true;
}

void MapDocument::BuildMapData() {
    mapData_.entities.clear();
    
    auto view = sceneManager_.GetScene().GetAllEntitiesWith<
        se::NameComponent, se::TransformComponent, PrimitiveFactory::EditorMetadata>();
    
    for (auto entityHandle : view) {
        se::Entity entity(entityHandle, sceneManager_.GetScenePtr());
        mapData_.entities.push_back(entityManager_.SerializeEntity(entity));
    }
    
    if (entityManager_.HasPlayerStart()) {
        auto playerStart = entityManager_.GetPlayerStart();
        auto& transform = playerStart.GetComponent<se::TransformComponent>();
        mapData_.hasPlayerStart = true;
        mapData_.playerStartPosition = transform.Position;
        mapData_.playerStartRotation = transform.Rotation;
    } else {
        mapData_.hasPlayerStart = false;
    }
    
    // Extract directional light data from scene
    auto lightView = sceneManager_.GetScene().GetAllEntitiesWith<
        se::TransformComponent, se::DirectionalLightComponent>();
    
    mapData_.hasDirectionalLight = false;
    for (auto entityHandle : lightView) {
        se::Entity entity(entityHandle, sceneManager_.GetScenePtr());
        auto& transform = entity.GetComponent<se::TransformComponent>();
        auto& light = entity.GetComponent<se::DirectionalLightComponent>();
        
        mapData_.hasDirectionalLight = true;
        mapData_.directionalLight.direction = -transform.GetForward();
        mapData_.directionalLight.position = transform.Position;
        mapData_.directionalLight.rotation = transform.Rotation;
        mapData_.directionalLight.color = light.Color;
        mapData_.directionalLight.intensity = light.Intensity;
        mapData_.directionalLight.castShadows = light.CastShadows;
        mapData_.directionalLight.enabled = light.Enabled;
        break;  // Only save the first light
    }
    
    SE_LOG_INFO("MapDocument: Built data with {} entities, light: {}", 
                mapData_.entities.size(), mapData_.hasDirectionalLight ? "Yes" : "No");
}

void MapDocument::LoadFromData(const MapData& data) {
    for (const auto& entityData : data.entities) {
        entityManager_.CreateFromData(entityData);
    }
    
    if (data.hasPlayerStart) {
        auto playerStart = entityManager_.CreatePlayerStart();
        auto& transform = playerStart.GetComponent<se::TransformComponent>();
        transform.SetPosition(data.playerStartPosition);
        transform.SetRotation(data.playerStartRotation);
        SE_LOG_INFO("MapDocument: Loaded Player Start at ({}, {}, {})",
                    data.playerStartPosition.x, data.playerStartPosition.y, data.playerStartPosition.z);
    }
    
    // Apply loaded light settings to editor light
    if (data.hasDirectionalLight) {
        auto lightView = sceneManager_.GetScene().GetAllEntitiesWith<
            se::TransformComponent, se::DirectionalLightComponent>();
        
        for (auto entityHandle : lightView) {
            se::Entity entity(entityHandle, sceneManager_.GetScenePtr());
            auto& transform = entity.GetComponent<se::TransformComponent>();
            auto& light = entity.GetComponent<se::DirectionalLightComponent>();
            
            transform.SetPosition(data.directionalLight.position);
            transform.SetRotation(data.directionalLight.rotation);
            light.Color = data.directionalLight.color;
            light.Intensity = data.directionalLight.intensity;
            light.CastShadows = data.directionalLight.castShadows;
            light.Enabled = data.directionalLight.enabled;
            
            SE_LOG_INFO("MapDocument: Loaded light with intensity {}", data.directionalLight.intensity);
            break;
        }
    }
}

}  // namespace mst
