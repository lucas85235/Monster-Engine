#include "engine/ecs/LightSyncSystem.h"

#include <cmath>

#include "engine/Log.h"
#include "engine/core/ServiceLocator.h"
#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/renderer/LightSystem.h"

#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

namespace se {

namespace {

// Compute forward direction from TransformComponent Euler angles (degrees)
Vector3 ComputeForward(const TransformComponent& transform) {
    float yawRad   = glm::radians(transform.Rotation.y);
    float pitchRad = glm::radians(transform.Rotation.x);

    Vector3 forward;
    forward.x = cos(pitchRad) * sin(yawRad);
    forward.y = sin(pitchRad);
    forward.z = cos(pitchRad) * cos(yawRad);
    return glm::normalize(forward);
}

}  // anonymous namespace

void LightSyncSystem::Sync(Scene& scene) {
    auto* lightSystem = ServiceLocator::Get().GetLightSystemPtr();
    if (!lightSystem) return;

    auto& registry = scene.GetRegistry();

    // ─── Directional Lights ──────────────────────────────────────
    {
        auto view = registry.view<DirectionalLightComponent, TransformComponent>();
        for (auto entity : view) {
            auto& light     = view.get<DirectionalLightComponent>(entity);
            auto& transform = view.get<TransformComponent>(entity);

            if (!light.Enabled) continue;

            // Derive direction from TransformComponent rotation (Euler degrees)
            Vector3 forward = ComputeForward(transform);

            lightSystem->SetDirectionalLight(
                forward.x, forward.y, forward.z,
                light.Color.x, light.Color.y, light.Color.z,
                light.Intensity,
                light.CastShadows
            );

            // Only one directional light supported
            break;
        }
    }

    // ─── Point Lights ────────────────────────────────────────────
    // Simple clear-and-rebuild approach per frame.
    // A more sophisticated system would track additions/removals.
    {
        auto pointView = registry.view<PointLightComponent, TransformComponent>();
        bool hasPointLights = pointView.begin() != pointView.end();

        if (hasPointLights) {
            // We need to clear and re-add all lights (including directional)
            lightSystem->ClearLights();

            // Re-add directional light after clear
            auto dirView = registry.view<DirectionalLightComponent, TransformComponent>();
            for (auto entity : dirView) {
                auto& light     = dirView.get<DirectionalLightComponent>(entity);
                auto& transform = dirView.get<TransformComponent>(entity);
                if (!light.Enabled) continue;

                Vector3 forward = ComputeForward(transform);
                lightSystem->SetDirectionalLight(
                    forward.x, forward.y, forward.z,
                    light.Color.x, light.Color.y, light.Color.z,
                    light.Intensity,
                    light.CastShadows
                );
                break;
            }

            // Add all point lights
            for (auto entity : pointView) {
                auto& light     = pointView.get<PointLightComponent>(entity);
                auto& transform = pointView.get<TransformComponent>(entity);

                if (!light.Enabled) continue;

                light.InternalIndex = lightSystem->AddPointLight(
                    transform.Position.x, transform.Position.y, transform.Position.z,
                    light.Color.x, light.Color.y, light.Color.z,
                    light.Intensity,
                    light.Falloff
                );
            }
        }
    }
}

}  // namespace se
