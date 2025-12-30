#include "EntityCommands.h"

#include <string>

#include "../core/EditorContext.h"
#include "engine/Log.h"

namespace mst {

// ============================================================================
// CreatePrimitiveCommand
// ============================================================================

CreatePrimitiveCommand::CreatePrimitiveCommand(EditorContext& ctx, PrimitiveType type, const std::string& name)
    : ctx_(ctx), type_(type), name_(name) {}

void CreatePrimitiveCommand::Execute() {
    auto& entityManager = ctx_.GetEntityManager();
    se::Entity entity;
    
    if (createdHandle_ != entt::null) {
        // Re-create from saved data
        entity = entityManager.CreateFromData(savedData_);
    } else {
        // First execution - create new
        entity = entityManager.CreatePrimitive(type_, name_);
        if (entity.IsValid()) {
            createdHandle_ = entity.GetHandle();
            savedData_ = entityManager.SerializeEntity(entity);
        }
    }
    
    if (entity.IsValid()) {
        ctx_.GetSelection().ClearSelection();
        ctx_.GetSelection().Select(entity);
        ctx_.GetDocument().MarkDirty();
    }
    
    SE_LOG_INFO("CreatePrimitiveCommand::Execute - created entity");
}

void CreatePrimitiveCommand::Undo() {
    if (createdHandle_ == entt::null) return;
    
    auto& scene = ctx_.GetScene();
    se::Entity entity(createdHandle_, &scene);
    
    if (entity.IsValid()) {
        ctx_.GetSelection().ClearSelection();
        ctx_.GetEntityManager().DeleteEntity(entity);
        SE_LOG_INFO("CreatePrimitiveCommand::Undo - deleted entity");
    }
}

std::string CreatePrimitiveCommand::GetName() const {
    return std::string("Create ") + PrimitiveTypeToString(type_);
}

// ============================================================================
// DeleteEntitiesCommand
// ============================================================================

DeleteEntitiesCommand::DeleteEntitiesCommand(EditorContext& ctx, const std::vector<se::Entity>& entities)
    : ctx_(ctx) {
    // Save entity data before deletion
    for (const auto& entity : entities) {
        if (entity.IsValid()) {
            savedData_.push_back(ctx_.GetEntityManager().SerializeEntity(entity));
            deletedHandles_.push_back(entity.GetHandle());
        }
    }
}

void DeleteEntitiesCommand::Execute() {
    auto& scene = ctx_.GetScene();
    
    for (auto handle : deletedHandles_) {
        se::Entity entity(handle, &scene);
        if (entity.IsValid()) {
            ctx_.GetEntityManager().DeleteEntity(entity);
        }
    }
    
    ctx_.GetDocument().MarkDirty();
    SE_LOG_INFO("DeleteEntitiesCommand::Execute - deleted {} entities", deletedHandles_.size());
}

void DeleteEntitiesCommand::Undo() {
    auto& entityManager = ctx_.GetEntityManager();
    
    // Recreate entities from saved data
    for (const auto& data : savedData_) {
        se::Entity entity = entityManager.CreateFromData(data);
        if (entity.IsValid()) {
            ctx_.GetSelection().Select(entity);
        }
    }
    
    ctx_.GetDocument().MarkDirty();
    SE_LOG_INFO("DeleteEntitiesCommand::Undo - restored {} entities", savedData_.size());
}

std::string DeleteEntitiesCommand::GetName() const {
    if (savedData_.size() == 1) {
        return "Delete " + savedData_[0].name;
    }
    return "Delete " + std::to_string(savedData_.size()) + " entities";
}

// ============================================================================
// TransformEntityCommand
// ============================================================================

TransformEntityCommand::TransformEntityCommand(EditorContext& ctx, se::Entity entity,
                                                const se::TransformComponent& oldTransform,
                                                const se::TransformComponent& newTransform)
    : ctx_(ctx), entityHandle_(entity.GetHandle()), oldTransform_(oldTransform), newTransform_(newTransform) {}

void TransformEntityCommand::Execute() {
    auto& scene = ctx_.GetScene();
    se::Entity entity(entityHandle_, &scene);
    
    if (entity.IsValid() && entity.HasComponent<se::TransformComponent>()) {
        auto& transform = entity.GetComponent<se::TransformComponent>();
        transform = newTransform_;
        ctx_.GetDocument().MarkDirty();
    }
}

void TransformEntityCommand::Undo() {
    auto& scene = ctx_.GetScene();
    se::Entity entity(entityHandle_, &scene);
    
    if (entity.IsValid() && entity.HasComponent<se::TransformComponent>()) {
        auto& transform = entity.GetComponent<se::TransformComponent>();
        transform = oldTransform_;
        ctx_.GetDocument().MarkDirty();
        SE_LOG_INFO("TransformEntityCommand::Undo");
    }
}

std::string TransformEntityCommand::GetName() const {
    return "Transform Entity";
}

bool TransformEntityCommand::CanMerge(const ICommand& other) const {
    const auto* otherTransform = dynamic_cast<const TransformEntityCommand*>(&other);
    if (!otherTransform) return false;
    return entityHandle_ == otherTransform->entityHandle_;
}

void TransformEntityCommand::Merge(const ICommand& other) {
    const auto& otherTransform = static_cast<const TransformEntityCommand&>(other);
    newTransform_ = otherTransform.newTransform_;
}

// ============================================================================
// DuplicateEntityCommand
// ============================================================================

DuplicateEntityCommand::DuplicateEntityCommand(EditorContext& ctx, se::Entity source)
    : ctx_(ctx), sourceHandle_(source.GetHandle()) {}

void DuplicateEntityCommand::Execute() {
    auto& scene = ctx_.GetScene();
    auto& entityManager = ctx_.GetEntityManager();
    
    se::Entity entity;
    
    if (createdHandle_ != entt::null) {
        // Re-create from saved data
        entity = entityManager.CreateFromData(savedData_);
    } else {
        // First execution
        se::Entity source(sourceHandle_, &scene);
        if (source.IsValid()) {
            entity = entityManager.DuplicateEntity(source);
            if (entity.IsValid()) {
                createdHandle_ = entity.GetHandle();
                savedData_ = entityManager.SerializeEntity(entity);
            }
        }
    }
    
    if (entity.IsValid()) {
        ctx_.GetSelection().ClearSelection();
        ctx_.GetSelection().Select(entity);
        ctx_.GetDocument().MarkDirty();
    }
    
    SE_LOG_INFO("DuplicateEntityCommand::Execute");
}

void DuplicateEntityCommand::Undo() {
    if (createdHandle_ == entt::null) return;
    
    auto& scene = ctx_.GetScene();
    se::Entity entity(createdHandle_, &scene);
    
    if (entity.IsValid()) {
        ctx_.GetSelection().ClearSelection();
        ctx_.GetEntityManager().DeleteEntity(entity);
        SE_LOG_INFO("DuplicateEntityCommand::Undo");
    }
}

std::string DuplicateEntityCommand::GetName() const {
    return "Duplicate Entity";
}

}  // namespace mst
