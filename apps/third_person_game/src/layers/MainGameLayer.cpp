#include "MainGameLayer.h"

#include "../../../SampleUtilities.h"
#include "../components/CharacterController.h"
#include "../components/CharacterRender.h"
#include "engine/Application.h"
#include "engine/Camera.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/input/InputManager.h"
#include "engine/physics/PhysicsDebugDraw.h"
#include "engine/physics/PhysicsSystem.h"
#include "engine/resources/MeshManager.h"

namespace FirstGame {
void ImguiDebug() {
    auto& app    = se::Application::Get();
    auto& window = app.GetWindow();

    ImGui::Begin("Main Game Debug");
    ImGui::Text("Debug Info");
    ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
    ImGui::Text("Current Resolution: [%d x %d]", window.GetWidth(), window.GetHeight());
    ImGui::Separator();
    ImGui::End();
}

MainGameLayer::~MainGameLayer() = default;

void MainGameLayer::OnAttach() {
    Layer::OnAttach();
    scene_ = CreateScope<Scene>("Main Game", SceneSettings{.EnablePhysics = true});

    // Set as active scene so Components can access it via Application::Get().GetActiveScene()
    Application::Get().SetActiveScene(scene_.get());

    // Create character entity and add Character + CharacterController components
    // Both inherit from Component, so lifecycle methods are called automatically
    character_entity_ = scene_->CreateEntity("Character");
    character_entity_.AddComponent<Character>();
    character_entity_.AddComponent<CharacterController>();
    character_entity_.AddComponent<CharacterRender>();
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