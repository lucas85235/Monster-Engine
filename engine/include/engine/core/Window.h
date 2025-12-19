#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "Engine.h"
#include "engine/events/EventBus.h"

struct GLFWwindow;

namespace se {

class GraphicsContext;

struct WindowSpec {
    std::string           Title      = "Simple-Engine";
    uint32_t              Width      = 800;
    uint32_t              Height     = 600;
    bool                  Decorated  = true;
    bool                  Fullscreen = false;
    bool                  VSync      = true;
    EventBus*             AppEventBus   = nullptr;
    std::filesystem::path IconPath;
};

class Window {
   public:
    explicit Window(const WindowSpec& spec);
    ~Window();

    void OnUpdate();  // Poll events
    void Init();

    uint32_t GetWidth() const { return spec_.Width; }
    uint32_t GetHeight() const { return spec_.Height; }

    void SetWidth(uint32_t width) { spec_.Width = width; }
    void SetHeight(uint32_t height) { spec_.Height = height; }

    void SetVSync(bool enabled);
    void SetTitle(const std::string& title);

    bool IsVSync() const { return vsync_; }

    bool ShouldClose() const;
    void RequestClose() const;

    WindowHandle GetNativeWindow() const { return window_handle_; }

    void SwapBuffers() const;

    static Window* Create(const WindowSpec& specification);

   private:
    void Shutdown();

    static void FramebufferSizeCallback(WindowHandle window, int width, int height);

   private:
    WindowHandle           window_handle_ = nullptr;
    Scope<GraphicsContext> context_;
    WindowSpec             spec_;
    bool                   vsync_ = true;

    inline static EventBus* event_bus_ = nullptr;
};

}  // namespace se