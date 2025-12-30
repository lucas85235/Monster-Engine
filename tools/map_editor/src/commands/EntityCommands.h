#pragma once
/**
 * EntityCommands.h - Undoable entity operations.
 *
 * Implements Create, Delete, and Transform commands for the undo/redo system.
 */

#include <vector>

#include "ICommand.h"
#include "../core/MapData.h"
#include "../core/PrimitiveFactory.h"
#include "engine/ecs/Entity.h"
#include "engine/ecs/SimpleComponents.h"

namespace mst {

class EditorContext;

// Command to create a primitive entity
class CreatePrimitiveCommand : public ICommand {
public:
    CreatePrimitiveCommand(EditorContext& ctx, PrimitiveType type, const std::string& name = "");
    
    void Execute() override;
    void Undo() override;
    std::string GetName() const override;

private:
    EditorContext& ctx_;
    PrimitiveType type_;
    std::string name_;
    entt::entity createdHandle_ = entt::null;
    MapEntityData savedData_;
};

// Command to delete one or more entities
class DeleteEntitiesCommand : public ICommand {
public:
    DeleteEntitiesCommand(EditorContext& ctx, const std::vector<se::Entity>& entities);
    
    void Execute() override;
    void Undo() override;
    std::string GetName() const override;

private:
    EditorContext& ctx_;
    std::vector<MapEntityData> savedData_;
    std::vector<entt::entity> deletedHandles_;
};

// Command to transform an entity (position, rotation, scale)
class TransformEntityCommand : public ICommand {
public:
    TransformEntityCommand(EditorContext& ctx, se::Entity entity,
                           const se::TransformComponent& oldTransform,
                           const se::TransformComponent& newTransform);
    
    void Execute() override;
    void Undo() override;
    std::string GetName() const override;
    
    bool CanMerge(const ICommand& other) const override;
    void Merge(const ICommand& other) override;
    
    entt::entity GetEntityHandle() const { return entityHandle_; }

private:
    EditorContext& ctx_;
    entt::entity entityHandle_;
    se::TransformComponent oldTransform_;
    se::TransformComponent newTransform_;
};

// Command to duplicate an entity
class DuplicateEntityCommand : public ICommand {
public:
    DuplicateEntityCommand(EditorContext& ctx, se::Entity source);
    
    void Execute() override;
    void Undo() override;
    std::string GetName() const override;

private:
    EditorContext& ctx_;
    entt::entity sourceHandle_;
    entt::entity createdHandle_ = entt::null;
    MapEntityData savedData_;
};

}  // namespace mst
