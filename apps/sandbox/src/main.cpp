#include <engine/Application.h>
#include <engine/Log.h>

#include "AppLayer.h"

using namespace std;

int main() {
    se::ApplicationSpec appSpec;
    appSpec.Name         = "Simple engine";
    appSpec.WindowWidth  = 1920;
    appSpec.WindowHeight = 1080;

    se::Application application(appSpec);
    application.Run();
}
