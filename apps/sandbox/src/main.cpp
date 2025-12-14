#include <engine/Application.h>
#include <engine/Log.h>

#include "AppLayer.h"
#include "ThirdPersonLayer.h"
#include "event_sample/EventSampleLayer.h"
#include "input_sample/InputSampleLayer.h"
#include "narrator_demo/AIChatLayer.h"
#include "narrator_demo/NarratorLayer.h"
#include "UILayer.h"
#include "engine/ui/RmlUiLayer.h"
#include "physics_sample/PhysicsSampleLayer.h"

using namespace std;
using namespace se;

int main() {
    ApplicationSpecification appSpec;
    appSpec.Name         = "Simple Engine - Stanley Parable Narrator Demo";
    appSpec.WindowWidth  = 1280;
    appSpec.WindowHeight = 720;

    Application application(appSpec);
    // application.PushOverlay<RmlUiLayer>();
    // application.PushLayer<UILayer>();
    // application.PushLayer<AppLayer>();
    // application.PushLayer<ThirdPersonLayer>();
    // application.PushLayer<PhysicsSampleLayer>();
    // application.PushLayer<EventSampleLayer>();
    // application.PushLayer<InputSampleLayer>();
    // application.PushLayer<AIChatLayer>();
    application.PushLayer<NarratorLayer>();
    application.Run();
}
