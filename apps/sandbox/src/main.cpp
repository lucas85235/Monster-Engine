#include <engine/Application.h>
#include <engine/Log.h>
#include "ThirdPersonLayer.h"

using namespace se;

int main() {
    ApplicationSpecification appSpec;
    appSpec.Name         = "Monster Engine - Third Person Demo";
    appSpec.WindowWidth  = 1280;
    appSpec.WindowHeight = 720;

    Application application(appSpec);

    // Push the third-person gameplay layer (Filament rendering + Bullet physics)
    application.PushLayer<ThirdPersonLayer>();

    SE_LOG_INFO("=== Third Person Demo (Filament) ===");
    SE_LOG_INFO("  WASD - Move");
    SE_LOG_INFO("  Mouse - Look around");
    SE_LOG_INFO("  Space - Jump");
    SE_LOG_INFO("  Shift - Sprint");
    SE_LOG_INFO("  E - Grab/Release objects");
    SE_LOG_INFO("  LMB - Shoot");
    SE_LOG_INFO("  Tab - Toggle mouse capture");
    SE_LOG_INFO("Close the window to exit.");

    return application.Run();
}
