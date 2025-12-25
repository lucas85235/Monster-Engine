#include "MapDocument.h"

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
    
    SE_LOG_INFO("MapDocument: Built data with {} entities", mapData_.entities.size());
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
}

}  // namespace mst
