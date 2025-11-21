#include <engine/Application.h>
#include <engine/Log.h>

#include "AppLayer.h"
#include "ThirdPersonLayer.h"
#include "event_sample/EventSampleLayer.h"
#include "input_sample/InputSampleLayer.h"
#include "UILayer.h"
#include "engine/ui/RmlUiLayer.h"

using namespace std;
using namespace se;

int main() {
    ApplicationSpecification appSpec;
    appSpec.Name         = "Simple engine - Third Person Demo";
    appSpec.WindowWidth  = 1280;
    appSpec.WindowHeight = 720;

    Application application(appSpec);
    application.PushOverlay<RmlUiLayer>();
    application.PushLayer<UILayer>();
    // application.PushLayer<AppLayer>();
    // application.PushLayer<ThirdPersonLayer>();
    // application.PushLayer<EventSampleLayer>();
    application.PushLayer<InputSampleLayer>();
    application.Run();
}
