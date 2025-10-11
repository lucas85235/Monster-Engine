#include "AppLayer.h"
#include <engine/Application.h>
#include <engine/Log.h>

int main() {
    se::ApplicationSpec appSpec;
    appSpec.Name = "Simple engine";
    appSpec.WindowWidth = 1920;
    appSpec.WindowHeight = 1080;

    se::LogInit(true);

    se::Application application(appSpec);
    application.PushLayer<AppLayer>();
    application.Run();
}
