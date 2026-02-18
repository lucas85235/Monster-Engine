#include "engine/ui/flutter/FlutterEmbedder.h"

#include <filesystem>

#import <FlutterMacOS/FlutterMacOS.h>

#include "engine/Log.h"
#include "engine/Window.h"
#include "engine/renderer/FilamentContext.h"
#include "engine/renderer/FilamentRenderer.h"
#include "engine/ui/flutter/FlutterOverlayRenderer.h"
#include "engine/ui/flutter/FlutterPlatformChannel.h"

namespace se::ui::flutter {

FlutterEmbedder::~FlutterEmbedder() {
    if (initialized_) {
        Shutdown();
    }
}

void FlutterEmbedder::Init(FilamentContext* context, FilamentRenderer* renderer,
                           Window* window, EventBus* eventBus,
                           const std::string& dartProjectPath) {
    if (initialized_) {
        SE_LOG_WARN("FlutterEmbedder::Init — already initialized");
        return;
    }

    context_           = context;
    renderer_          = renderer;
    window_            = window;
    event_bus_         = eventBus;
    dart_project_path_ = dartProjectPath;

    // Query current window dimensions.
    framebuffer_width_  = window_->GetWidth();
    framebuffer_height_ = window_->GetHeight();
    window_width_       = framebuffer_width_;
    window_height_      = framebuffer_height_;

    // ── Create Flutter Dart project ──────────────────────────
    NSString* assetsPath = nil;
    std::filesystem::path projectPath(dartProjectPath);

    // Look for flutter_assets directory
    if (std::filesystem::exists(projectPath / "flutter_assets")) {
        assetsPath = [NSString stringWithUTF8String:
            (projectPath / "flutter_assets").c_str()];
    } else if (std::filesystem::exists(projectPath)) {
        assetsPath = [NSString stringWithUTF8String:
            projectPath.c_str()];
    } else {
        SE_LOG_ERROR("FlutterEmbedder::Init — Dart project path not found: {}",
                     dartProjectPath);
        return;
    }

    FlutterDartProject* dartProject = [[FlutterDartProject alloc] init];
    // Set the assets path via the lookup paths accessible to FlutterDartProject.
    // The framework finds assets via bundle or specified paths.

    // ── Create Flutter engine ────────────────────────────────
    FlutterEngine* engine = [[FlutterEngine alloc]
        initWithName:@"MonsterEngine"
             project:dartProject
          allowHeadlessExecution:YES];

    BOOL success = [engine runWithEntrypoint:nil];
    if (!success) {
        SE_LOG_ERROR("FlutterEmbedder::Init — FlutterEngine runWithEntrypoint failed");
        return;
    }

    // Store as opaque pointer (bridge-retained to prevent ARC from releasing).
    engine_handle_ = (__bridge_retained void*)engine;

    // ── Initialize overlay renderer ──────────────────────────
    overlay_renderer_ = std::make_unique<FlutterOverlayRenderer>();
    overlay_renderer_->Init(context_->GetEngine(), context_->GetMainView(),
                            framebuffer_width_, framebuffer_height_);

    // ── Initialize platform channel ──────────────────────────
    platform_channel_ = std::make_unique<FlutterPlatformChannel>();
    platform_channel_->Init((__bridge void*)engine);

    initialized_ = true;
    SE_LOG_INFO("FlutterEmbedder initialized (FlutterMacOS, headless mode, "
                "Dart project: '{}')", dartProjectPath);
}

void FlutterEmbedder::Shutdown() {
    if (!initialized_) return;

    SE_LOG_INFO("FlutterEmbedder shutting down");

    platform_channel_->Shutdown();
    platform_channel_.reset();

    overlay_renderer_->Shutdown();
    overlay_renderer_.reset();

    // Shutdown and release the Flutter engine.
    if (engine_handle_) {
        FlutterEngine* engine = (__bridge_transfer FlutterEngine*)engine_handle_;
        [engine shutDownEngine];
        engine_handle_ = nullptr;
        // ARC will release the engine when 'engine' goes out of scope.
    }

    context_   = nullptr;
    renderer_  = nullptr;
    window_    = nullptr;
    event_bus_ = nullptr;

    initialized_ = false;
    SE_LOG_INFO("FlutterEmbedder shut down");
}

bool FlutterEmbedder::IsInitialized() const {
    return initialized_;
}

void FlutterEmbedder::OnResize(uint32_t fbWidth, uint32_t fbHeight,
                               uint32_t winWidth, uint32_t winHeight) {
    if (!initialized_) return;

    framebuffer_width_  = fbWidth;
    framebuffer_height_ = fbHeight;
    window_width_       = winWidth;
    window_height_      = winHeight;

    if (overlay_renderer_) {
        overlay_renderer_->OnResize(fbWidth, fbHeight);
    }
}

void FlutterEmbedder::BeginFrame(float deltaTimeSeconds) {
    if (!initialized_) return;
    // In headless mode, the engine runs on its own run loop.
    // The overlay renderer is driven by the Filament frame cycle.
}

void FlutterEmbedder::EndFrame() {
    if (!initialized_) return;
    // Overlay renderer commits any pending texture uploads.
}

// ── Input forwarding ──────────────────────────────────────────
// Note: In headless mode without a FlutterViewController, input
// events need to be sent via the binary messenger protocol.
// For now, we log them; full input integration requires either
// attaching a FlutterViewController or using the platform channel.

void FlutterEmbedder::OnMouseMove(double x, double y) {
    if (!initialized_) return;
    last_mouse_x_ = x;
    last_mouse_y_ = y;
}

void FlutterEmbedder::OnMouseButton(int button, bool pressed) {
    if (!initialized_) return;
    mouse_is_down_ = pressed;
}

void FlutterEmbedder::OnMouseScroll(double xOffset, double yOffset) {
    if (!initialized_) return;
    // Scroll events forwarded via platform channel if needed.
}

void FlutterEmbedder::OnKey(int key, int scancode, int action, int mods) {
    if (!initialized_) return;
    // Key events forwarded via platform channel if needed.
}

void FlutterEmbedder::OnTextInput(uint32_t codepoint) {
    if (!initialized_) return;
    // Text input forwarded via platform channel if needed.
}

// ── Platform channel access ───────────────────────────────────

FlutterPlatformChannel& FlutterEmbedder::GetPlatformChannel() {
    return *platform_channel_;
}

}  // namespace se::ui::flutter
