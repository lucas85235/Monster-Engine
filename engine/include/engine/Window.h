#pragma once

#include <cstdint>
#include <memory>
#include <string>

#include "Engine.h"
#include "event/Event.h"
#include "new_event_system/EventBus.h"

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
    EventBus*             EventBus;
    std::filesystem::path IconPath;
};

class Window {
   public:
    explicit Window(const WindowSpec& spec);
    Window(uint32_t width, uint32_t height, const std::string& title);
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

    bool ShouldClose() const;
    void RequestClose() const;

    WindowHandle GetNativeWindow() const {
        return window_handle_;
    }

    void SwapBuffers() const;

    static Window* Create(const WindowSpec& specification);

    virtual void SetEventCallback(const EventCallbackFn& callback) {
        window_data_.EventCallback = callback;
    }

   private:
    void Shutdown();

    static void FramebufferSizeCallback(WindowHandle window, int width, int height);

    static void WindowResizeEvent();

   private:
    WindowHandle           window_handle_ = nullptr;
    Scope<GraphicsContext> context_;
    WindowSpec             spec_;

    struct WindowData {
        std::string Title;
        uint32_t    Width, Height;

        EventCallbackFn EventCallback;
    };

    WindowData window_data_;
    bool       vsync_ = true;

    inline static EventBus* event_bus_;
};

}  // namespace se