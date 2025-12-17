#include <iostream>

#include "engine/Application.h"
#include "layers/MainGameLayer.h"

using namespace se;
int main() {
    ApplicationSpecification app_spec{.Fullscreen      = false,
                                      .Name            = "First-Game",
                                      .Resizable       = true,
                                      .WindowDecorated = true,
                                      .Fullscreen      = true,
                                      .WindowWidth     = 800,
                                      .WindowHeight    = 600};

    Application app{app_spec};
    app.PushLayer<FirstGame::MainGameLayer>();
    app.Run();

    return 0;
}