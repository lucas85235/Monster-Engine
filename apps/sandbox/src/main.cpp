#include <engine/core/Application.h>
#include <engine/core/Log.h>

#include "VulkanTestLayer.h"

// Disabled for Vulkan minimal testing:
// #include "AppLayer.h"
// #include "ThirdPersonLayer.h"
// #include "UILayer.h"
// #include "engine/ui/RmlUiLayer.h"
// #include "event_sample/EventSampleLayer.h"
// #include "input_sample/InputSampleLayer.h"
// #include "physics_sample/PhysicsSampleLayer.h"

using namespace std;
using namespace se;

int main() {
    ApplicationSpecification appSpec;
    appSpec.Name         = "Simple engine - Vulkan Test";
    appSpec.WindowWidth  = 1280;
    appSpec.WindowHeight = 720;
    appSpec.GraphicsApi  = RHI::API::Vulkan;  // Enable Vulkan for testing

    Application application(appSpec);
    application.PushLayer<VulkanTestLayer>();
    application.Run();
}
