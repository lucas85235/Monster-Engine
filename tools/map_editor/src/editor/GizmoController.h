#pragma once

#include "Engine.h"
#include "engine/Camera.h"
#include "engine/ecs/SimpleComponents.h"

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

    bool IsUsing() const { return isUsing_; }

    Operation GetOperation() const { return operation_; }
    Space     GetSpace() const { return space_; }

    const char* GetOperationName() const;
    const char* GetSpaceName() const;

   private:
    Operation operation_ = Operation::Translate;
    Space     space_     = Space::World;
    bool      isUsing_   = false;
};

}  // namespace mst
