#include "engine/ecs/Scene.h"

#include "engine/Application.h"
#include "engine/Camera.h"
#include "engine/Log.h"
#include "engine/core/ServiceLocator.h"
#include "engine/ecs/AnimationSystem.h"
#include "engine/ecs/CameraSystem.h"
#include "engine/ecs/ComponentSystem.h"
#include "engine/ecs/FilamentComponents.h"
#include "engine/ecs/LightSyncSystem.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/physics/PhysicsSystem.h"
#include "engine/renderer/FilamentRenderBridge.h"

namespace se {

Scene::Scene(const std::string& name, const SceneSettings& settings) : name_(name) {
    SE_LOG_INFO("Scene '{}' created", name_);

    // Initialize ComponentSystem (always enabled)
    component_system_ = std::make_unique<ComponentSystem>(this);
    SE_LOG_INFO("Scene '{}' component system enabled", name_);

    if (settings.EnablePhysics) {
        physics_system_ = std::make_unique<PhysicsSystem>(this);
        physics_system_->Initialize();
        
        // Register pre-tick callback to run FixedUpdate in sync with each Bullet substep
        // This ensures game logic runs at the exact same rate as physics
        physics_system_->SetPreTickCallback([this](float fixedDt) {
            if (component_system_) {
                component_system_->RunFixedUpdateOnce(fixedDt);
            }
        });
        
        SE_LOG_INFO("Scene '{}' physics enabled with synced FixedUpdate callback", name_);
    }
}

Scene::~Scene() {
    Clear();

    // Shutdown ComponentSystem first (components may reference physics)
    if (component_system_) {
        component_system_.reset();
        SE_LOG_INFO("Scene '{}' component system shutdown", name_);
    }

    if (physics_system_) {
        physics_system_->Shutdown();
        SE_LOG_INFO("Scene '{}' physics shutdown", name_);
    }

    SE_LOG_INFO("Scene '{}' destroyed", name_);
}

Entity Scene::CreateEntity(const std::string& name) {
    Entity entity(registry_.create(), this);

    entity.AddComponent<TransformComponent>();
    entity.AddComponent<NameComponent>(name.empty() ? "Entity" : name);

    SE_LOG_INFO("Entity '{}' created with ID: {}", name, entity.GetID());

    return entity;
}

void Scene::DestroyEntity(Entity entity) {
    if (!entity.IsValid()) {
        SE_LOG_WARN("Attempted to destroy invalid entity");
        return;
    }

    auto& nameComp = entity.GetComponent<NameComponent>();
    SE_LOG_INFO("Entity '{}' destroyed", nameComp.Name);

    // Release associated Filament renderable before removing ECS entity.
    if (registry_.any_of<FilamentRenderableComponent>(entity.GetHandle())) {
        auto& renderable = registry_.get<FilamentRenderableComponent>(entity.GetHandle());
        if (renderable.handle.IsValid()) {
            if (auto* meshSystem = ServiceLocator::Get().GetMeshSystemPtr()) {
                meshSystem->DestroyRenderable(renderable.handle);
            }
        }
    }

    registry_.destroy(entity.GetHandle());
}

Entity Scene::FindEntityByName(const std::string& name) {
    auto view = registry_.view<NameComponent>();

    for (auto entity : view) {
        auto& nameComp = view.get<NameComponent>(entity);
        if (nameComp.Name == name) { return Entity(entity, this); }
    }

    SE_LOG_WARN("Entity with name '{}' not found", name);
    return Entity();
}

static void UpdateTransformRecursive(entt::registry& registry, entt::entity entity, const Matrix4& parentMatrix) {
    if (!registry.valid(entity)) return;

    // Check if entity has TransformComponent
    if (!registry.any_of<TransformComponent>(entity)) return;

    auto& transform = registry.get<TransformComponent>(entity);
    Matrix4 localMatrix = transform.GetTransform();
    transform.WorldMatrix = parentMatrix * localMatrix;

    // Propagate to children
    if (registry.any_of<RelationshipComponent>(entity)) {
        const auto& rel = registry.get<RelationshipComponent>(entity);
        for (auto child : rel.Children) {
            UpdateTransformRecursive(registry, child, transform.WorldMatrix);
        }
    }
}

void Scene::UpdateTransforms() {
    auto view = registry_.view<TransformComponent>();
    for (auto entity : view) {
        // If entity has parent, skip (it will be updated by its parent)
        if (registry_.any_of<RelationshipComponent>(entity)) {
             if (registry_.get<RelationshipComponent>(entity).Parent != entt::null) {
                 continue;
             }
        }
        
        // Root entity (no parent)
        auto& transform = view.get<TransformComponent>(entity);
        transform.WorldMatrix = transform.GetTransform(); // Local is World for root
        
        // Propagate to children if any
        if (registry_.any_of<RelationshipComponent>(entity)) {
            const auto& rel = registry_.get<RelationshipComponent>(entity);
            for (auto child : rel.Children) {
                UpdateTransformRecursive(registry_, child, transform.WorldMatrix);
            }
        }
    }
}

void Scene::OnUpdate(float deltaTime) {
    // Process pending component starts before physics
    if (component_system_) {
        component_system_->ProcessPendingStarts();
    }
    
    // NOTE: FixedUpdate is now called via Bullet's pre-tick callback
    // This ensures game logic runs in perfect sync with each physics substep

    // Physics simulation - internally calls our FixedUpdate via pre-tick callback
    if (physics_system_) { physics_system_->Update(deltaTime); }

    // Store delta time for CameraSystem (used during OnRender)
    last_delta_time_ = deltaTime;

    // Animation system - tick all AnimatorComponents
    AnimationSystem::Update(*this, deltaTime);

    // Update transforms BEFORE component Update() so components can read current WorldMatrix
    UpdateTransforms();

    // Component Update and LateUpdate (variable rate logic)
    if (component_system_) {
        component_system_->Update(deltaTime);
        component_system_->LateUpdate(deltaTime);
    }
    
    // Update transforms AFTER logic to ensure any component changes are reflected for rendering
    UpdateTransforms();

    // Bone attachment system - must happen AFTER character world matrices are updated
    // to avoid a 1-frame lag/flickering.
    AnimationSystem::UpdateBoneAttachments(*this);

    // Update transforms again ONLY if we have attachments to ensure they are rendered correctly.
    // In a more complex engine, this would be part of a dependency-sorted update.
    UpdateTransforms();
}

void Scene::OnRender(const Camera& camera, float aspectRatio) {
    (void)aspectRatio;

    // Sync ECS world transforms to Filament renderables.
    FilamentRenderBridge::SyncScene(*this);

    if (physics_system_) { physics_system_->RenderDebug(camera); }
}

void Scene::OnRender() {
    // Update ECS camera system before rendering
    CameraSystem::Update(*this, last_delta_time_);

    // Sync ECS lights to Filament
    LightSyncSystem::Sync(*this);

    if (!active_camera_) {
        // Filament transform sync does not require an active Camera.
        FilamentRenderBridge::SyncScene(*this);
        return;
    }

    auto& window      = Application::Get().GetWindow();
    float aspectRatio = static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight());

    OnRender(*active_camera_, aspectRatio);
}

void Scene::Clear() {
    SE_LOG_INFO("Clearing scene '{}'", name_);

    // Ensure all Filament handles tracked by ECS are released.
    if (auto* meshSystem = ServiceLocator::Get().GetMeshSystemPtr()) {
        auto view = registry_.view<FilamentRenderableComponent>();
        for (auto entity : view) {
            auto& renderable = view.get<FilamentRenderableComponent>(entity);
            if (renderable.handle.IsValid()) {
                meshSystem->DestroyRenderable(renderable.handle);
            }
        }
    }

    registry_.clear();
}

Entity Scene::CreateCamera(const std::string& name, CameraMode mode) {
    auto entity = CreateEntity(name);

    auto& cam = entity.AddComponent<CameraComponent>(mode);
    cam.IsMain = true;

    // ThirdPerson and Orbit modes need a SpringArmComponent
    if (mode == CameraMode::ThirdPerson || mode == CameraMode::Orbit) {
        entity.AddComponent<SpringArmComponent>();
    }

    SE_LOG_INFO("Camera entity '{}' created with mode {}", name, static_cast<int>(mode));
    return entity;
}

Entity Scene::CreateDirectionalLight(const std::string& name, float intensity) {
    auto entity = CreateEntity(name);

    auto& light     = entity.AddComponent<DirectionalLightComponent>();
    light.Intensity = intensity;
    light.Color     = {1.0f, 0.95f, 0.9f};  // Warm white default (sun-like)

    // Point the light downward by default (sun-like direction)
    auto& transform = entity.GetComponent<TransformComponent>();
    transform.SetRotation({-45.0f, -30.0f, 0.0f});

    SE_LOG_INFO("Directional light '{}' created with intensity {}", name, intensity);
    return entity;
}

Entity Scene::CreatePointLight(const std::string& name, float intensity) {
    auto entity = CreateEntity(name);

    auto& light     = entity.AddComponent<PointLightComponent>();
    light.Intensity = intensity;

    SE_LOG_INFO("Point light '{}' created with intensity {}", name, intensity);
    return entity;
}

}  // namespace se

