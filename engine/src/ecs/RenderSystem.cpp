#include "engine/ecs/RenderSystem.h"

#include "engine/Log.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/ecs/Scene.h"
#include "engine/renderer/SceneRenderer.h"

namespace se {
bool RenderSystem::initialized_ = false;

void RenderSystem::Init() {
    if (initialized_) {
        SE_LOG_WARN("RenderSystem already initialized");
        return;
    }

    SE_LOG_INFO("Initializing RenderSystem");
    initialized_ = true;
}

void RenderSystem::Shutdown() {
    if (!initialized_) return;

    SE_LOG_INFO("Shutting down RenderSystem");
    initialized_ = false;
}

void RenderSystem::Render(Scene& scene, const Camera& camera, float aspectRatio) {
    if (!initialized_) {
        SE_LOG_ERROR("RenderSystem not initialized!");
        return;
    }

    // Configure lighting
    SceneRenderer::ClearDirectionalLight();
    SceneRenderer::DirectionalLightData lightData;
    auto                                lightView = scene.GetAllEntitiesWith<TransformComponent, DirectionalLightComponent>();
    for (auto entity : lightView) {
        auto& transform = lightView.get<TransformComponent>(entity);
        auto& light     = lightView.get<DirectionalLightComponent>(entity);

        if (!light.Enabled) continue;

        glm::vec3 direction = -transform.GetForward();
        if (glm::length(direction) <= 0.0f) { direction = glm::vec3(0.0f, -1.0f, 0.0f); }

        lightData.Direction   = glm::normalize(direction);
        lightData.Position    = transform.Position;
        lightData.Color       = light.Color;
        lightData.Intensity   = glm::max(light.Intensity, 0.0f);
        lightData.CastShadows = light.CastShadows;
        lightData.Active      = true;
        SceneRenderer::SetDirectionalLight(lightData);
        break;
    }

    // Begin scene rendering
    glm::mat4 projection = camera.getProjectionMatrix(aspectRatio);
    SceneRenderer::BeginScene(camera, projection);

    // Get all entities with TransformComponent and MeshRenderComponent
    auto view = scene.GetAllEntitiesWith<TransformComponent, MeshRenderComponent>();

    int renderedCount = 0;
    int skippedCount  = 0;

    // Render each entity
    for (auto entity : view) {
        auto& transform  = view.get<TransformComponent>(entity);
        auto& meshRender = view.get<MeshRenderComponent>(entity);

        // Skip if not visible
        if (!meshRender.IsVisible) {
            skippedCount++;
            continue;
        }

        // Skip if missing vertex array or material
        if (!meshRender.VertexArray || !meshRender.Material) {
            SE_LOG_WARN("Entity missing VertexArray or Material!");
            skippedCount++;
            continue;
        }

        // Debug: Log the first entity's transform
        static int debugCount = 0;
        if (debugCount < 1) {
            auto pos = transform.Position;
            SE_LOG_INFO("First Entity Transform - Pos: ({}, {}, {})", pos.x, pos.y, pos.z);
            SE_LOG_INFO("Camera Position: ({}, {}, {})", camera.GetPosition().x, camera.GetPosition().y, camera.GetPosition().z);
            SE_LOG_INFO("Projection Matrix: aspectRatio = {}", aspectRatio);
            debugCount++;
        }

        // Submit to renderer
        SceneRenderer::Submit(meshRender.VertexArray, meshRender.Material, transform.GetTransform(), meshRender.CastShadows,
                              meshRender.ReceiveShadows);
        renderedCount++;
    }

    // Always log on first 10 frames
    static int frameCount = 0;
    if (frameCount < 10) {
        SE_LOG_INFO("RenderSystem Frame {}: Rendered {} entities, Skipped {}", frameCount, renderedCount, skippedCount);
        frameCount++;
    }

    // End scene rendering
    SceneRenderer::EndScene();
}
}  // namespace se
