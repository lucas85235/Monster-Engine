#pragma once
/**
 * EntityManager.h - Single Responsibility for entity CRUD operations.
 *
 * Encapsulates all entity creation, duplication, and deletion logic.
 * Works with CommandSystem for undoable operations.
 */

#include <string>
#include <vector>

#include "Engine.h"
#include "MapData.h"
#include "PrimitiveFactory.h"
#include "engine/ecs/Entity.h"

namespace mst {

class SceneManager;
class EventBus;

class EntityManager {
public:
    EntityManager(SceneManager& sceneManager, EventBus& eventBus);
    
    se::Entity CreatePrimitive(PrimitiveType type, const std::string& name = "");
    se::Entity CreateFromData(const MapEntityData& data);
    
    se::Entity DuplicateEntity(se::Entity source);
    
    void DeleteEntity(se::Entity entity);
    void DeleteEntities(const std::vector<se::Entity>& entities);
    
    se::Entity GetPlayerStart() const { return playerStart_; }
    se::Entity CreatePlayerStart();
    bool HasPlayerStart() const { return playerStart_.IsValid(); }
    void ClearPlayerStart() { playerStart_ = se::Entity(); }
    
    MapEntityData SerializeEntity(se::Entity entity) const;

private:
    SceneManager& sceneManager_;
    EventBus& eventBus_;
    se::Entity playerStart_;
};

}  // namespace mst
