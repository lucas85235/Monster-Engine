#include <iostream>

#include "engine/Application.h"

using namespace se;
int main() {
    ApplicationSpecification app_spec{.Fullscreen      = false,
                                      .Name            = "First-Game",
                                      .Resizable       = true,
                                      .WindowDecorated = true,
                                      .Fullscreen      = false,
                                      .WindowWidth     = 800,
                                      .WindowHeight    = 600};

    Application app{app_spec};
    app.Run();

    return 0;
}