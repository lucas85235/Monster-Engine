#pragma once
/**
 * MapDocument.h - Current map state with dirty tracking and file operations.
 *
 * Encapsulates the map data, file path, and modified state.
 * Handles save/load operations through MapSerializer.
 */

#include <string>

#include "MapData.h"

namespace mst {

class EntityManager;
class SceneManager;
class EventBus;

class MapDocument {
public:
    MapDocument(EntityManager& entityManager, SceneManager& sceneManager, EventBus& eventBus);
    
    void New();
    bool Open(const std::string& path);
    bool Save();
    bool SaveAs(const std::string& path);
    
    bool IsDirty() const { return isDirty_; }
    void MarkDirty() { isDirty_ = true; }
    void ClearDirty() { isDirty_ = false; }
    
    const std::string& GetFilePath() const { return filePath_; }
    const std::string& GetMapName() const { return mapData_.mapName; }
    void SetMapName(const std::string& name) { mapData_.mapName = name; }
    
    const MapData& GetMapData() const { return mapData_; }

private:
    void BuildMapData();
    void LoadFromData(const MapData& data);
    
    EntityManager& entityManager_;
    SceneManager& sceneManager_;
    EventBus& eventBus_;
    
    MapData mapData_;
    std::string filePath_;
    bool isDirty_ = false;
};

}  // namespace mst
