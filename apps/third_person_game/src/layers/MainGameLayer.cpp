#include "MainGameLayer.h"

#include "../../../SampleUtilities.h"
#include "../components/CameraController.h"
#include "../components/Character.h"
#include "../components/CharacterController.h"
#include "../components/CharacterRender.h"
#include "engine/Application.h"
#include "engine/Camera.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/input/InputManager.h"
#include "engine/physics/PhysicsDebugDraw.h"
#include "engine/physics/PhysicsSystem.h"
#include "engine/resources/MeshManager.h"
#include "engine/resources/MapLoader.h"

namespace FirstGame {

void ImguiDebug() {
    auto& app    = se::Application::Get();
    auto& window = app.GetWindow();

    ImGui::Begin("Main Game Debug");
    ImGui::Text("Debug Info");
    ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
    ImGui::Text("Current Resolution: [%d x %d]", window.GetWidth(), window.GetHeight());
    ImGui::Separator();
    ImGui::Text("Press TAB to toggle mouse capture");
    ImGui::End();
}

MainGameLayer::~MainGameLayer() = default;

void MainGameLayer::OnAttach() {
    Layer::OnAttach();
    scene_ = CreateScope<Scene>("Main Game", SceneSettings{.EnablePhysics = true});

    Application::Get().SetActiveScene(scene_.get());

    // Load map from file
    auto mapResult = se::MapLoader::Load(*scene_, "assets/maps/test.mstmap");
    if (mapResult.success) {
        SE_LOG_INFO("Loaded map with {} entities", mapResult.entityCount);
    } else {
        SE_LOG_WARN("Failed to load map, creating empty scene");
    }

    // Create character entity with all components
    // Components are added in order and their Awake() is called immediately
    // Start() is called before first Update()
    character_entity_ = scene_->CreateEntity("Character");

    // 1. Character: Sets up physics (collider + rigidbody)
    character_entity_.AddComponent<Character>();

    // 2. CameraController: Sets up spring arm camera
    character_entity_.AddComponent<CameraController>();

    // 3. CharacterController: Binds input and coordinates Character + Camera
    character_entity_.AddComponent<CharacterController>();

    // 4. CharacterRender: Sets up mesh and materials
    character_entity_.AddComponent<CharacterRender>();

    // 5. Position character at Player Start if available
    if (mapResult.hasPlayerStart) {
        auto& transform = character_entity_.GetComponent<se::TransformComponent>();
        transform.SetPosition(mapResult.playerStartPosition);
        transform.SetRotation(mapResult.playerStartRotation);
        SE_LOG_INFO("Character spawned at Player Start: ({}, {}, {})",
                    mapResult.playerStartPosition.x,
                    mapResult.playerStartPosition.y,
                    mapResult.playerStartPosition.z);
    }
}

void MainGameLayer::OnDetach() {
    Application::Get().SetActiveScene(nullptr);
    Layer::OnDetach();
}

void MainGameLayer::OnUpdate(float ts) {
    Layer::OnUpdate(ts);
    scene_->OnUpdate(ts);
}

void MainGameLayer::OnRender() {
    Layer::OnRender();
    scene_->OnRender();
}

void MainGameLayer::OnImGuiRender() {
    Layer::OnImGuiRender();
    ImguiDebug();
}

} // namespace FirstGame