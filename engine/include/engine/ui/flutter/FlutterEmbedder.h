#pragma once

#include <cstdint>
#include <memory>
#include <string>

namespace se {
class FilamentContext;
class FilamentRenderer;
class Window;
class EventBus;
}  // namespace se

namespace se::ui::flutter {

// Full includes required because unique_ptr destructor needs complete type.
}  // namespace se::ui::flutter

#include "engine/ui/flutter/FlutterOverlayRenderer.h"
#include "engine/ui/flutter/FlutterPlatformChannel.h"

namespace se::ui::flutter {

/**
 * Main orchestrator for the Flutter integration on macOS.
 *
 * Uses the FlutterMacOS.framework Objective-C API (FlutterEngine,
 * FlutterViewController, FlutterMethodChannel) under the hood, bridged
 * via Objective-C++ in the .mm implementation file.
 *
 * Owns the Flutter engine lifecycle, forwards input events from GLFW,
 * and coordinates the overlay renderer and platform channel subsystems.
 *
 * On macOS, Flutter renders into a CALayer managed by FlutterViewController.
 * The overlay renderer captures this output for Filament compositing.
 *
 * Init/Shutdown must be called from the main thread.
 * BeginFrame/EndFrame must be called within the Filament frame bracket.
 */
class FlutterEmbedder {
public:
    FlutterEmbedder() = default;
    ~FlutterEmbedder();

    // Disallow copy/move.
    FlutterEmbedder(const FlutterEmbedder&) = delete;
    FlutterEmbedder& operator=(const FlutterEmbedder&) = delete;

    /**
     * Initialize the Flutter engine and overlay renderer.
     *
     * @param context          Active Filament context.
     * @param renderer         Filament renderer for overlay registration.
     * @param window           GLFW window (for size queries).
     * @param eventBus         Engine event bus (for input event listening).
     * @param dartProjectPath  Path to the Flutter/Dart project containing
     *                         the compiled Dart assets (flutter_assets/).
     */
    void Init(FilamentContext* context, FilamentRenderer* renderer,
              Window* window, EventBus* eventBus,
              const std::string& dartProjectPath);

    /** Shutdown the Flutter engine and release all resources. */
    void Shutdown();

    /** @return true after Init succeeds, false after Shutdown. */
    bool IsInitialized() const;

    /**
     * Handle a window/framebuffer resize.
     *
     * Updates both the overlay renderer and sends a metrics event
     * to the Flutter engine.
     */
    void OnResize(uint32_t fbWidth, uint32_t fbHeight,
                  uint32_t winWidth, uint32_t winHeight);

    /**
     * Begin a Flutter frame.
     *
     * Runs pending Flutter tasks and prepares the overlay.
     * Must be called after FilamentRenderer::BeginFrame().
     */
    void BeginFrame(float deltaTimeSeconds);

    /**
     * End the Flutter frame.
     *
     * Commits any pending work to the overlay renderer.
     */
    void EndFrame();

    // ── Input forwarding ──────────────────────────────────────

    void OnMouseMove(double x, double y);
    void OnMouseButton(int button, bool pressed);
    void OnMouseScroll(double xOffset, double yOffset);
    void OnKey(int key, int scancode, int action, int mods);
    void OnTextInput(uint32_t codepoint);

    // ── Platform channel access ───────────────────────────────

    /** @return Reference to the platform channel for Dart↔C++ messaging. */
    FlutterPlatformChannel& GetPlatformChannel();

private:
    // Opaque pointer to Objective-C implementation (PIMPL for Obj-C).
    // Defined in FlutterEmbedder.mm as an __bridge-retained FlutterEngine*.
    void* engine_handle_ = nullptr;

    std::unique_ptr<FlutterOverlayRenderer> overlay_renderer_;
    std::unique_ptr<FlutterPlatformChannel> platform_channel_;

    // Non-owning dependencies.
    FilamentContext*  context_  = nullptr;
    FilamentRenderer* renderer_ = nullptr;
    Window*           window_   = nullptr;
    EventBus*         event_bus_ = nullptr;

    // Dart project path.
    std::string dart_project_path_;

    // Input state tracking.
    double last_mouse_x_ = 0.0;
    double last_mouse_y_ = 0.0;
    bool   mouse_is_down_ = false;

    // Window dimensions for metrics events.
    uint32_t framebuffer_width_  = 0;
    uint32_t framebuffer_height_ = 0;
    uint32_t window_width_       = 0;
    uint32_t window_height_      = 0;

    bool initialized_ = false;
};

}  // namespace se::ui::flutter
