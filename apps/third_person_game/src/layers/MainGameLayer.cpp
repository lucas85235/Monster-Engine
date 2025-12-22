#include "MainGameLayer.h"

#include "engine/Application.h"

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

    scene_     = CreateScope<Scene>("Main Game");
    character_ = CreateRef<Character>(scene_->CreateEntity("Character"));
}

void MainGameLayer::OnDetach() {
    Layer::OnDetach();
}

void MainGameLayer::OnUpdate(float ts) {
    Layer::OnUpdate(ts);
}

void MainGameLayer::OnRender() {
    Layer::OnRender();
}

void MainGameLayer::OnImGuiRender() {
    Layer::OnImGuiRender();

    ImguiDebug();
}
}  // namespace FirstGame