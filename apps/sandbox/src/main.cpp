#include <engine/Application.h>
#include <engine/Log.h>

#include "AppLayer.h"
#include "event_sample/EventSampleLayer.h"
#include "input_sample/InputSampleLayer.h"

using namespace std;

int main() {
    ApplicationSpecification appSpec;
    appSpec.Name         = "Simple engine";
    appSpec.WindowWidth  = 800;
    appSpec.WindowHeight = 600;

    Application application(appSpec);
    application.PushLayer<AppLayer>();
    // application.PushLayer<EventSampleLayer>();
    // application.PushLayer<InputSampleLayer>();
    application.Run();
}
