#include "DemoTestLayer.h"

#include <glm.hpp>

#include "engine/Application.h"
#include "engine/Log.h"
#include "engine/core/ServiceLocator.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/ecs/FilamentComponents.h"
#include "engine/input/InputManager.h"
#include "engine/renderer/MaterialSystem.h"
#include "engine/renderer/MeshSystem.h"
#include "engine/renderer/MeshData.h"

namespace DemoTest {

using namespace se;

DemoTestLayer::~DemoTestLayer() {}

void DemoTestLayer::OnAttach() {
    Layer::OnAttach();
    SE_LOG_INFO("DemoTestLayer attached");

    // ─── Create Scene ────────────────────────────────────────────
    scene_ = std::make_shared<Scene>("Demo Scene");

    // Register with Application so the EditorLayer can discover it
    Application::Get().SetActiveScene(scene_.get());

    // ─── Camera (1 line!) ────────────────────────────────────────
    auto camera = scene_->CreateCamera("Main Camera", CameraMode::FreeFly);
    camera.GetComponent<TransformComponent>().SetPosition({0.0f, 3.0f, 10.0f});

    // ─── Directional Light (1 line!) ─────────────────────────────
    scene_->CreateDirectionalLight("Sun", 110000.0f);

    // ─── Materials ───────────────────────────────────────────────
    auto& materials = ServiceLocator::Get().GetMaterialSystem();
    auto& meshes    = ServiceLocator::Get().GetMeshSystem();

    auto defaultMat = materials.GetDefaultLit();

    MaterialConfig floorConfig;
    floorConfig.baseColor[0] = 0.3f;
    floorConfig.baseColor[1] = 0.3f;
    floorConfig.baseColor[2] = 0.35f;
    floorConfig.roughness    = 0.8f;
    auto floorMat = materials.CreateMaterial(floorConfig);

    MaterialConfig redConfig;
    redConfig.baseColor[0] = 0.8f;
    redConfig.baseColor[1] = 0.2f;
    redConfig.baseColor[2] = 0.2f;
    redConfig.metallic     = 0.3f;
    redConfig.roughness    = 0.4f;
    auto redMat = materials.CreateMaterial(redConfig);

    MaterialConfig goldConfig;
    goldConfig.baseColor[0] = 0.9f;
    goldConfig.baseColor[1] = 0.7f;
    goldConfig.baseColor[2] = 0.2f;
    goldConfig.metallic     = 0.8f;
    goldConfig.roughness    = 0.3f;
    auto goldMat = materials.CreateMaterial(goldConfig);

    // ─── Floor ───────────────────────────────────────────────────
    {
        auto floor = scene_->CreateEntity("Floor");
        floor.GetComponent<TransformComponent>().SetPosition({0.0f, -0.5f, 0.0f});
        floor.GetComponent<TransformComponent>().SetScale({30.0f, 1.0f, 30.0f});

        auto meshData = MeshPrimitives::CreateBox(1.0f, 1.0f, 1.0f);
        auto renderable = meshes.CreateRenderable(meshData, floorMat, false);
        floor.AddComponent<FilamentRenderableComponent>(renderable);
    }

    // ─── Cubes ───────────────────────────────────────────────────
    {
        auto cube = scene_->CreateEntity("Red Cube");
        cube.GetComponent<TransformComponent>().SetPosition({-3.0f, 1.0f, 0.0f});

        auto meshData = MeshPrimitives::CreateBox(1.5f, 1.5f, 1.5f);
        auto renderable = meshes.CreateRenderable(meshData, redMat);
        cube.AddComponent<FilamentRenderableComponent>(renderable);
    }

    {
        auto cube = scene_->CreateEntity("Gold Cube");
        cube.GetComponent<TransformComponent>().SetPosition({3.0f, 1.0f, 0.0f});

        auto meshData = MeshPrimitives::CreateBox(1.5f, 1.5f, 1.5f);
        auto renderable = meshes.CreateRenderable(meshData, goldMat);
        cube.AddComponent<FilamentRenderableComponent>(renderable);
    }

    // ─── Sphere ──────────────────────────────────────────────────
    {
        auto sphere = scene_->CreateEntity("Sphere");
        sphere.GetComponent<TransformComponent>().SetPosition({0.0f, 2.0f, -3.0f});

        auto meshData = MeshPrimitives::CreateSphere(1.0f);
        auto renderable = meshes.CreateRenderable(meshData, defaultMat);
        sphere.AddComponent<FilamentRenderableComponent>(renderable);
    }

    // ─── Input Bindings ──────────────────────────────────────────
    auto& input = InputManager::Get();
    input.CreateActionMap(inputMapName_);
    input.PushContext(inputMapName_);

    // Camera movement
    input.BindAxis(inputMapName_, "MoveForward", Key::W, 1.0f);
    input.BindAxis(inputMapName_, "MoveForward", Key::S, -1.0f);
    input.BindAxis(inputMapName_, "MoveRight", Key::D, 1.0f);
    input.BindAxis(inputMapName_, "MoveRight", Key::A, -1.0f);
    input.BindAxis(inputMapName_, "MoveUp", Key::Space, 1.0f);
    input.BindAxis(inputMapName_, "MoveUp", Key::LeftControl, -1.0f);

    // Camera look
    input.BindAxis(inputMapName_, "LookX", Key::MouseX, 1.0f);
    input.BindAxis(inputMapName_, "LookY", Key::MouseY, -1.0f);

    // Sprint
    input.BindAction(inputMapName_, "Sprint", Key::LeftShift);

    // Toggle mouse
    input.BindAction(inputMapName_, "ToggleCursor", Key::Tab);

    input.SetCursorMode(CursorMode::Locked);

    SE_LOG_INFO("Demo scene ready — {} entities", scene_->GetEntityCount());
}

void DemoTestLayer::OnDetach() {
    Layer::OnDetach();

    auto& input = InputManager::Get();
    input.PopContext(inputMapName_);
    input.RemoveActionMap(inputMapName_);

    scene_.reset();
    SE_LOG_INFO("DemoTestLayer detached");
}

void DemoTestLayer::OnUpdate(float ts) {
    animationTime_ += ts;

    // Toggle mouse capture
    if (InputManager::Get().IsActionJustPressed("ToggleCursor")) {
        static bool locked = true;
        locked = !locked;
        InputManager::Get().SetCursorMode(locked ? CursorMode::Locked : CursorMode::Normal);
    }

    // Animate entities
    auto view = scene_->GetAllEntitiesWith<TransformComponent, NameComponent>();
    for (auto entity : view) {
        auto& transform = view.get<TransformComponent>(entity);
        auto& name      = view.get<NameComponent>(entity);

        if (name.Name == "Red Cube") {
            transform.Rotate({0.0f, 45.0f * ts, 0.0f});
        }

        if (name.Name == "Sphere") {
            float bounce = glm::sin(animationTime_ * 2.0f) * 0.5f;
            transform.Position.y = 2.0f + bounce;
            transform.MarkDirty();
        }
    }

    // Update scene systems (physics, components, transforms)
    scene_->OnUpdate(ts);
}

void DemoTestLayer::OnRender() {
    scene_->OnRender();
}

}  // namespace DemoTest
