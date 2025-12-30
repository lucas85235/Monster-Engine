#pragma once
/**
 * GizmoController.h - Transform manipulation gizmo wrapper.
 *
 * Wraps ImGuizmo for translate/rotate/scale operations.
 * Supports world and local space modes with keyboard shortcuts.
 * Tracks transform changes for undo/redo support.
 */

#include "Engine.h"
#include "engine/Camera.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/ecs/Entity.h"

namespace mst {

using se::Vector3;
using se::Vector4;
using se::Matrix4;

class GizmoController {
   public:
    enum class Operation { Translate, Rotate, Scale };
    enum class Space { Local, World };

    GizmoController() = default;

    void SetOperation(Operation op);
    void SetSpace(Space space);
    void ToggleSpace();
    void CycleOperation();

    bool Manipulate(const Camera& camera, float aspectRatio, se::TransformComponent& transform);
    
    // For undo support: call when starting manipulation
    void BeginManipulation(se::Entity entity, const se::TransformComponent& transform);
    
    // For undo support: returns true if manipulation just ended and we have a command to create
    bool EndedManipulation() const { return justEnded_; }
    void ClearEndedFlag() { justEnded_ = false; }
    
    // Get data for creating undo command
    se::Entity GetManipulatedEntity() const { return manipulatedEntity_; }
    const se::TransformComponent& GetOriginalTransform() const { return originalTransform_; }

    bool IsUsing() const { return isUsing_; }
    bool WasUsing() const { return wasUsing_; }

    Operation GetOperation() const { return operation_; }
    Space     GetSpace() const { return space_; }

    const char* GetOperationName() const;
    const char* GetSpaceName() const;

   private:
    Operation operation_ = Operation::Translate;
    Space     space_     = Space::World;
    bool      isUsing_   = false;
    bool      wasUsing_  = false;
    bool      justEnded_ = false;
    
    // For undo support
    se::Entity manipulatedEntity_;
    se::TransformComponent originalTransform_;
    bool hasOriginal_ = false;
};

}  // namespace mst
