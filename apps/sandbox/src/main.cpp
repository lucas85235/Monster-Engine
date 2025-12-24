#include <engine/core/Application.h>
#include <engine/core/Log.h>

// Vulkan Test
// #include "VulkanTestLayer.h"

// Full engine layers
#include "ThirdPersonLayer.h"

// Instancing test
#include "InstancedCubesLayer.h"

using namespace std;
using namespace se;

int main() {
    ApplicationSpecification appSpec;
    appSpec.Name         = "Monster Engine - Instanced Cubes Test";
    appSpec.WindowWidth  = 1280;
    appSpec.WindowHeight = 720;
    appSpec.GraphicsApi  = RHI::API::Vulkan;  // Enable Vulkan for testing

    Application application(appSpec);
    application.PushLayer<ThirdPersonLayer>();
    application.Run();
}
