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

    // Sync transforms: ECS WorldMatrix → Filament TransformManager
    auto view = scene.GetRegistry().view<TransformComponent, FilamentRenderableComponent>();
    for (auto entity : view) {
        auto& transform  = view.get<TransformComponent>(entity);
        auto& renderable = view.get<FilamentRenderableComponent>(entity);

        if (!renderable.autoSync || !renderable.handle.IsValid()) continue;

        meshSystem->SetTransform(renderable.handle, glm::value_ptr(transform.WorldMatrix));
    }

    // Sync materials: MeshRenderComponent PBR params → Filament MaterialHandle
    auto materialView = scene.GetRegistry().view<MeshRenderComponent, FilamentRenderableComponent>();
    for (auto entity : materialView) {
        auto& mesh       = materialView.get<MeshRenderComponent>(entity);
        auto& renderable = materialView.get<FilamentRenderableComponent>(entity);

        if (!mesh.UseCustomPBR || !renderable.materialHandle.IsValid()) continue;

        renderable.materialHandle.SetColor(mesh.Color.r, mesh.Color.g, mesh.Color.b, mesh.Color.a);
        renderable.materialHandle.SetMetallic(mesh.Metallic);
        renderable.materialHandle.SetRoughness(mesh.Roughness);
        renderable.materialHandle.SetReflectance(mesh.Reflectance);

        if (mesh.EmissiveFactor > 0.0f) {
            renderable.materialHandle.SetEmissive(
                mesh.EmissiveColor.r, mesh.EmissiveColor.g, mesh.EmissiveColor.b,
                mesh.EmissiveFactor);
        }
    }
}

}  // namespace se

