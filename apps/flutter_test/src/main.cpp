#include <engine/Application.h>
#include <engine/Log.h>

#if defined(SE_ENABLE_FLUTTER) && SE_ENABLE_FLUTTER
#include "FlutterTestLayer.h"
#endif

using namespace se;

int main() {
    ApplicationSpecification appSpec;
    appSpec.Name         = "Monster Engine - Flutter Test";
    appSpec.WindowWidth  = 1280;
    appSpec.WindowHeight = 720;
    appSpec.EnableImGui  = true;

#if defined(SE_ENABLE_FLUTTER) && SE_ENABLE_FLUTTER
    appSpec.EnableFlutter    = true;
    // Path to the Flutter/Dart test project assets.
    // In development, this points to the build output of the test Dart project.
    // The AOT snapshot or kernel_blob.bin should be at this path.
    appSpec.FlutterProjectPath = PROJECT_SOURCE_DIR "/apps/flutter_test/flutter_app/build";
#endif

    Application application(appSpec);

#if defined(SE_ENABLE_FLUTTER) && SE_ENABLE_FLUTTER
    application.PushLayer<FlutterTestLayer>();
    SE_LOG_INFO("=== Flutter Integration Test ===");
    SE_LOG_INFO("  Flutter embedder: {}", application.GetFlutterEmbedder() ? "ACTIVE" : "INACTIVE");
    SE_LOG_INFO("  Platform channel 'monster/test' registered");
    SE_LOG_INFO("  Sending periodic ticks to Dart UI");
#else
    SE_LOG_WARN("=== Flutter Test ===");
    SE_LOG_WARN("  Flutter is DISABLED (SE_ENABLE_FLUTTER=OFF)");
    SE_LOG_WARN("  Rebuild with: cmake -DSE_ENABLE_FLUTTER=ON ..");
#endif

    SE_LOG_INFO("Close the window to exit.");

    return application.Run();
}
