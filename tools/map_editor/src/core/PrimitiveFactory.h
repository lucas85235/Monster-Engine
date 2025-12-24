#pragma once

#include <memory>

#include "Engine.h"
#include "core/MapData.h"
#include "engine/ecs/Entity.h"
#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/renderer/Material.h"
#include "engine/renderer/VertexArray.h"

namespace mst {

class PrimitiveFactory {
   public:
    struct EditorMetadata {
        PrimitiveType primitiveType = PrimitiveType::Cube;
        bool          hasCollision  = false;
        ColliderType  colliderType  = ColliderType::None;
        Vector3       colliderSize{1.0f, 1.0f, 1.0f};
        float         colliderRadius = 0.5f;
        float         colliderHeight = 1.0f;
    };

    static se::Entity CreatePrimitive(se::Scene& scene, PrimitiveType type,
                                       const std::string& name = "");

    static std::shared_ptr<se::VertexArray> GetPrimitiveMesh(PrimitiveType type);

    static std::shared_ptr<se::Material> GetDefaultMaterial();

   private:
    static uint32_t primitiveCounter_;
    static std::shared_ptr<se::Material> cachedMaterial_;
};

}  // namespace mst
