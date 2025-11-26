#include "AppLayer.h"
#include "engine/physics/PhysicsWorld.h"

#include <engine/Application.h>
#include <engine/Log.h>

using namespace std;

int main() {
    se::ApplicationSpec appSpec;
    appSpec.Name = "Simple engine";
    appSpec.WindowWidth = 1920;
    appSpec.WindowHeight = 1080;

    se::LogInit(true);

    auto physics_world = new se::PhysicsWorld();
    physics_world->Initialize();

    se::Application application(appSpec);
    application.PushLayer<AppLayer>();
    application.Run();
}
