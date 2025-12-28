#include <iostream>
#include "engine/Application.h"
#include "layers/MainGameLayer.h"

using namespace se;

int main() {
    ApplicationSpecification app_spec{.Name = "First-Game",
                                      .WindowWidth = 800,
                                      .WindowHeight = 600,
                                      .WindowDecorated = true,
                                      .Fullscreen = false,
                                      .VSync = false,
                                      .Resizable = true};

    Application app{app_spec};
    app.PushLayer<FirstGame::MainGameLayer>();
    app.Run();

    return 0;
}