#include "engine/renderer/FilamentRenderBridge.h"

#include "engine/core/ServiceLocator.h"
#include "engine/ecs/FilamentComponents.h"
#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"

#include <gtc/type_ptr.hpp>

namespace se {

void FilamentRenderBridge::SyncScene(Scene& scene) {
    MeshSystem* meshSystem = ServiceLocator::Get().GetMeshSystemPtr();
    if (!meshSystem) return;

    auto view = scene.GetRegistry().view<TransformComponent, FilamentRenderableComponent>();
    for (auto entity : view) {
        auto& transform  = view.get<TransformComponent>(entity);
        auto& renderable = view.get<FilamentRenderableComponent>(entity);

        if (!renderable.autoSync || !renderable.handle.IsValid()) continue;

        meshSystem->SetTransform(renderable.handle, glm::value_ptr(transform.WorldMatrix));
    }
}

}  // namespace se

