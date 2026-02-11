#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <chrono>

#include "Engine.h"
#include "engine/events/EventBus.h"

struct GLFWwindow;

namespace se {

struct WindowSpec {
    std::string           Title      = "Simple-Engine";
    uint32_t              Width      = 800;
    uint32_t              Height     = 600;
    bool                  Decorated  = true;
    bool                  Fullscreen = false;
    bool                  VSync      = true;
    EventBus*             event_bus   = nullptr;
    bool                  Resizable  = true;
    std::filesystem::path IconPath;
};

/**
 * Window class that uses GLFW with NO OpenGL context.
 * Rendering is handled entirely by Filament (Metal/Vulkan backend).
 */
class Window {
   public:
    explicit Window(const WindowSpec& spec);
    ~Window();

    void OnUpdate();  // Poll events
    void Init();

    uint32_t GetWidth() const {
        return spec_.Width;
    }
    uint32_t GetHeight() const {
        return spec_.Height;
    }

    void SetWidth(uint32_t width) {
        spec_.Width = width;
    }
    void SetHeight(uint32_t height) {
        spec_.Height = height;
    }

    void SetVSync(bool enabled);
    void SetTitle(const std::string& title);

    bool IsVSync() const {
        return vsync_;
    }
    
    // FPS Limiting (0 = unlimited)
    void SetTargetFPS(int fps);
    int GetTargetFPS() const { return target_fps_; }
    void ApplyFrameRateLimit();

    bool ShouldClose() const;
    void RequestClose() const;

    WindowHandle GetNativeWindow() const {
        return window_handle_;
    }

    static Window* Create(const WindowSpec& specification);

   private:
    void Shutdown();

    static void FramebufferSizeCallback(WindowHandle window, int width, int height);

   private:
    WindowHandle           window_handle_ = nullptr;
    WindowSpec             spec_;
    bool                   vsync_ = true;
    
    // FPS limiting
    int target_fps_ = 0;  // 0 = unlimited
    std::chrono::high_resolution_clock::time_point last_frame_time_;

    inline static EventBus* event_bus_ = nullptr;
};

}  // namespace se