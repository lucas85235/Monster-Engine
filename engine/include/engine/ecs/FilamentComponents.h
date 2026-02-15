#pragma once

#include "engine/renderer/MeshSystem.h"

namespace se {

/**
 * ECS link to a Filament renderable managed by MeshSystem.
 *
 * When present, Scene render sync updates this handle transform from
 * TransformComponent::WorldMatrix each frame.
 */
struct FilamentRenderableComponent {
    RenderableHandle handle;
    bool autoSync = true;

    FilamentRenderableComponent() = default;
    explicit FilamentRenderableComponent(RenderableHandle renderable, bool autoSyncTransforms = true)
        : handle(renderable), autoSync(autoSyncTransforms) {}
};

}  // namespace se

