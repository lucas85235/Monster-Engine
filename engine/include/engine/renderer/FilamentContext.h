#pragma once

#include <cstdint>

// Forward declarations - avoid including heavy Filament headers in the engine header
struct GLFWwindow;

namespace filament {
class Engine;
class SwapChain;
class Renderer;
class Scene;
class View;
class Camera;
} // namespace filament

namespace utils {
class Entity;
} // namespace utils

namespace se {

/**
 * Manages the Filament graphics engine lifecycle and core objects.
 *
 * Replaces GraphicsContext by creating and owning the Filament Engine,
 * SwapChain, Renderer, and default View/Scene/Camera.
 *
 * Thread Safety: Not thread-safe. Must be called from the main thread.
 */
class FilamentContext {
public:
    /**
     * Supported rendering backends.
     * Maps to filament::backend::Backend values.
     */
    enum class Backend : uint8_t {
        Default = 0, // Platform default (Metal on macOS, Vulkan on Linux/Windows)
        OpenGL  = 1,
        Vulkan  = 2,
        Metal   = 3
    };

    FilamentContext() = default;
    ~FilamentContext();

    // Non-copyable, non-movable
    FilamentContext(const FilamentContext&) = delete;
    FilamentContext& operator=(const FilamentContext&) = delete;
    FilamentContext(FilamentContext&&) = delete;
    FilamentContext& operator=(FilamentContext&&) = delete;

    /**
     * Initialize the Filament engine and create core objects.
     *
     * @param window GLFW window handle to create the SwapChain from.
     * @param width  Initial framebuffer width.
     * @param height Initial framebuffer height.
     * @param backend The rendering backend to use.
     */
    void Init(GLFWwindow* window, uint32_t width, uint32_t height,
              Backend backend = Backend::Default);

    /**
     * Shutdown the Filament engine and release all resources.
     */
    void Shutdown();

    /**
     * Notify Filament of a window resize.
     *
     * @param width  New framebuffer width.
     * @param height New framebuffer height.
     */
    void OnResize(uint32_t width, uint32_t height);

    // --- Accessors ---

    filament::Engine*   GetEngine()   const { return engine_; }
    filament::SwapChain* GetSwapChain() const { return swap_chain_; }
    filament::Renderer* GetRenderer() const { return renderer_; }
    filament::Scene*    GetScene()    const { return scene_; }
    filament::View*     GetView()     const { return view_; }
    filament::Camera*   GetCamera()   const { return camera_; }

    bool IsInitialized() const { return engine_ != nullptr; }

private:
    /**
     * Get the native window handle from GLFW for the current platform.
     * On macOS with Metal: returns CAMetalLayer*
     * On macOS with OpenGL: returns NSView*
     * On Linux: returns X11 Window
     * On Windows: returns HWND
     */
    void* GetNativeWindowHandle(GLFWwindow* window) const;

    filament::Engine*    engine_     = nullptr;
    filament::SwapChain* swap_chain_ = nullptr;
    filament::Renderer*  renderer_   = nullptr;
    filament::Scene*     scene_      = nullptr;
    filament::View*      view_       = nullptr;
    filament::Camera*    camera_     = nullptr;

    // Filament uses its own Entity system for cameras
    utils::Entity*       camera_entity_ = nullptr;

    Backend              active_backend_ = Backend::Default;
};

} // namespace se
