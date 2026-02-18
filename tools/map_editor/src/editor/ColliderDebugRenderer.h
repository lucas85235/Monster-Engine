#pragma once
/**
 * ColliderDebugRenderer.h - Renders collider wireframes in the editor viewport.
 *
 * TODO: Re-implement using Filament MeshSystem with PrimitiveType::LINES.
 * Currently stubbed (no-op) after OpenGL removal.
 */

#include <glm.hpp>
#include <vector>

#include "engine/ecs/Entity.h"

namespace mst {

class ColliderDebugRenderer {
   public:
    ColliderDebugRenderer() = default;
    ~ColliderDebugRenderer() = default;

    void Init();
    void UpdateColliders(const std::vector<se::Entity>& entities);
    void Render(const glm::mat4& view, const glm::mat4& projection);

    void SetVisible(bool visible) { visible_ = visible; }
    bool IsVisible() const { return visible_; }

   private:
    bool visible_ = true;
};

}  // namespace mst
