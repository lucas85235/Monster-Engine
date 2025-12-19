#include <engine/core/Application.h>
#include <engine/core/Log.h>

#include "AppLayer.h"
#include "ThirdPersonLayer.h"
#include "UILayer.h"
#include "engine/ui/RmlUiLayer.h"
#include "event_sample/EventSampleLayer.h"
#include "input_sample/InputSampleLayer.h"
#include "physics_sample/PhysicsSampleLayer.h"

using namespace std;
using namespace se;

int main() {
    ApplicationSpecification appSpec;
    appSpec.Name         = "Simple engine - Third Person Demo";
    appSpec.WindowWidth  = 1280;
    appSpec.WindowHeight = 720;

    Application application(appSpec);
    application.PushLayer<ThirdPersonLayer>();
    application.Run();

    // application.PushOverlay<RmlUiLayer>();
    // application.PushLayer<UILayer>();
    // application.PushLayer<AppLayer>();
    // application.PushLayer<PhysicsSampleLayer>();
    // application.PushLayer<EventSampleLayer>();
    // application.PushLayer<InputSampleLayer>();
}
