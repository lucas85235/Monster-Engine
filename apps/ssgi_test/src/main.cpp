#include <iostream>
#include "engine/Application.h"
#include "layers/SSGITestLayer.h"

using namespace se;

int main() {
    ApplicationSpecification app_spec{
        .Name = "SSGI Test",
        .WindowWidth = 1280,
        .WindowHeight = 720,
        .WindowDecorated = true,
        .Fullscreen = false,
        .VSync = false,
        .Resizable = true
    };

    Application app{app_spec};
    app.PushLayer<SSGITest::SSGITestLayer>();
    app.Run();

    return 0;
}
