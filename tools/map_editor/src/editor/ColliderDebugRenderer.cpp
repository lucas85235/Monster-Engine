#include "editor/ColliderDebugRenderer.h"

#include "engine/Log.h"

namespace mst {

void ColliderDebugRenderer::Init() {
    // TODO: Create Filament wireframe renderables for collider debug shapes.
    SE_LOG_INFO("ColliderDebugRenderer: Init (Filament stub — collider debug disabled)");
}

void ColliderDebugRenderer::UpdateColliders(const std::vector<se::Entity>& /*entities*/) {
    // TODO: Re-create wireframe meshes for entities with collider metadata.
}

void ColliderDebugRenderer::Render(const glm::mat4& /*view*/, const glm::mat4& /*projection*/) {
    // TODO: Render collider wireframes via Filament. Currently a no-op.
}

}  // namespace mst
