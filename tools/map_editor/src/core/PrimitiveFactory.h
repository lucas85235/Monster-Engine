#pragma once
/**
 * PrimitiveFactory.h - Factory for creating editor primitive entities.
 *
 * Creates cube, sphere, capsule, cylinder, and plane entities with
 * mesh rendering and editor metadata for collision configuration.
 */

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
        
        // Rigidbody settings
        uint8_t       rigidbodyType = 0;  // 0=Static, 1=Dynamic, 2=Kinematic
        float         mass = 1.0f;
        
        // Material assignment
        std::string   materialName;
        std::string   compiledMaterialPath;  // Path to compiled .mstmat binary (self-contained)
        bool          hasCustomMaterial = false;
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
