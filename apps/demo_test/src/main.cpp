#include "engine/Application.h"
#include "engine/editor/EditorLayer.h"
#include "layers/DemoTestLayer.h"

int main() {
    se::ApplicationSpecification spec{};
    spec.Name            = "Monster Engine - Demo Test";
    spec.WindowWidth     = 1600;
    spec.WindowHeight    = 900;
    spec.VSync           = true;
    spec.StartMaximized  = true;
    spec.EnableImGui     = true;

    se::Application app(spec);

    // Push the game layer (creates scene and game logic)
    app.PushLayer<DemoTest::DemoTestLayer>();

    // Push the editor overlay (auto-discovers scene from Application)
    app.PushOverlay<se::EditorLayer>();

    return app.Run();
}
