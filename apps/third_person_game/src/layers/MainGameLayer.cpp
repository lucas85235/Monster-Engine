#include "MainGameLayer.h"

namespace FirstGame {
MainGameLayer::~MainGameLayer() {}
void MainGameLayer::OnAttach() {
    Layer::OnAttach();
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

    ImGui::Begin("Main Game");
    ImGui::Text("Hello");
    ImGui::End();
}
}  // namespace FirstGame