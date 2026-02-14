#include "engine/Window.h"

#include <GLFW/glfw3.h>

#include <stdexcept>
#include <thread>

#include "engine/Log.h"
#include "engine/core/ServiceLocator.h"
#include "engine/events/Events.h"

namespace se {

static bool s_GLFWInitialized = false;

static void GLFWErrorCallback(int error, const char* description) {
    SE_LOG_ERROR("GLFW Error ({}): {}", error, description);
}

Window::Window(const WindowSpec& spec) : spec_(spec) {}

Window::~Window() {
    Shutdown();
}

void Window::OnUpdate() {
    glfwPollEvents();
}

void Window::SetVSync(bool enabled) {
    // VSync is managed by Filament's SwapChain, not GLFW.
    // Store the value for the renderer to query.
    vsync_ = enabled;
}

void Window::SetTitle(const std::string& title) {
    spec_.Title = title;
    glfwSetWindowTitle(window_handle_, spec_.Title.c_str());
}

bool Window::ShouldClose() const {
    return glfwWindowShouldClose(window_handle_);
}

void Window::RequestClose() const {
    if (event_bus_) { event_bus_->Invoke<WindowCloseEvent>(); }
    glfwSetWindowShouldClose(window_handle_, GLFW_TRUE);
}

void Window::SetTargetFPS(int fps) {
    target_fps_ = fps;
}

void Window::ApplyFrameRateLimit() {
    if (target_fps_ <= 0) return;
    
    auto now = std::chrono::high_resolution_clock::now();
    auto frame_duration = std::chrono::duration<double>(1.0 / target_fps_);
    auto elapsed = now - last_frame_time_;
    
    if (elapsed < frame_duration) {
        auto sleep_time = frame_duration - elapsed;
        auto sleep_ms = std::chrono::duration_cast<std::chrono::milliseconds>(sleep_time);
        if (sleep_ms.count() > 0) {
            std::this_thread::sleep_for(sleep_ms);
        }
    }
    
    last_frame_time_ = std::chrono::high_resolution_clock::now();
}

Window* Window::Create(const WindowSpec& specification) {
    return new Window(specification);
}

void Window::Init() {
    event_bus_ = spec_.event_bus;

    if (!s_GLFWInitialized) {
        int success = glfwInit();
        if (!success) { throw std::runtime_error("Could not initialize GLFW!"); }
        glfwSetErrorCallback(GLFWErrorCallback);
        s_GLFWInitialized = true;
    }

    // No OpenGL context — Filament creates its own Metal/Vulkan context
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_DECORATED, spec_.Decorated);
    glfwWindowHint(GLFW_RESIZABLE, spec_.Resizable);

    SE_LOG_INFO("Creating window {} ({}, {}) [Filament backend]",
                spec_.Title, spec_.Width, spec_.Height);

    if (spec_.Fullscreen) {
        glfwWindowHint(GLFW_DECORATED, GLFW_FALSE);
        GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();

        int width, height;
        glfwGetMonitorWorkarea(primary_monitor, nullptr, nullptr, &width, &height);

        window_handle_ =
            glfwCreateWindow(width, height, spec_.Title.c_str(), primary_monitor, nullptr);
    } else {
        window_handle_ =
            glfwCreateWindow(spec_.Width, spec_.Height, spec_.Title.c_str(), nullptr, nullptr);

        int          width, height;
        GLFWmonitor* primary_monitor = glfwGetPrimaryMonitor();
        glfwGetMonitorWorkarea(primary_monitor, nullptr, nullptr, &width, &height);

        // Center the window on screen
        glfwSetWindowPos(window_handle_, width / 2 - spec_.Width / 2,
                         height / 2 - spec_.Height / 2);
    }

    if (!window_handle_) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    glfwSetWindowUserPointer(window_handle_, this);

    // Set callbacks
    glfwSetFramebufferSizeCallback(window_handle_, FramebufferSizeCallback);

    glfwSetKeyCallback(
        window_handle_, [](WindowHandle window, int key, int scancode, int action, int mods) {
            if (!event_bus_) return;
            if (key < 0) return;

            switch (action) {
                case GLFW_PRESS: {
                    event_bus_->Invoke<KeyPressedEvent>(static_cast<KeyCode>(key), 0);
                    break;
                }
                case GLFW_RELEASE: {
                    event_bus_->Invoke<KeyReleasedEvent>(static_cast<KeyCode>(key));
                    break;
                }
                case GLFW_REPEAT: {
                    event_bus_->Invoke<KeyPressedEvent>(static_cast<KeyCode>(key), 1);
                    break;
                }
            }
        });

    glfwSetCharCallback(window_handle_, [](WindowHandle window, unsigned int codepoint) {
        if (!event_bus_) return;
        event_bus_->Invoke<TextInputEvent>(codepoint);
        if (codepoint <= 0xFFFFu) {
            event_bus_->Invoke<KeyTypedEvent>(static_cast<KeyCode>(codepoint));
        }
    });

    glfwSetMouseButtonCallback(
        window_handle_, [](WindowHandle window, int button, int action, int mods) {
            if (!event_bus_) return;
            if (button < 0) return;

            switch (action) {
                case GLFW_PRESS: {
                    event_bus_->Invoke<MouseButtonPressedEvent>(static_cast<MouseButton>(button));
                    break;
                }
                case GLFW_RELEASE: {
                    event_bus_->Invoke<MouseButtonReleasedEvent>(static_cast<MouseButton>(button));
                    break;
                }
            }
        });

    glfwSetCursorPosCallback(window_handle_, [](WindowHandle window, double xpos, double ypos) {
        if (!event_bus_) return;
        event_bus_->Invoke<MouseMovedEvent>(static_cast<float>(xpos), static_cast<float>(ypos));
    });

    glfwSetScrollCallback(window_handle_, [](WindowHandle window, double xoffset, double yoffset) {
        if (!event_bus_) return;
        event_bus_->Invoke<MouseScrolledEvent>(static_cast<float>(xoffset),
                                               static_cast<float>(yoffset));
    });

    glfwSetWindowFocusCallback(window_handle_, [](WindowHandle window, int focused) {
        if (!event_bus_) return;
        event_bus_->Invoke<WindowFocusEvent>(focused == GLFW_TRUE);
    });

    SE_LOG_INFO("Window created successfully (no OpenGL context)");
}

void Window::Shutdown() {
    if (window_handle_) {
        glfwDestroyWindow(window_handle_);
        window_handle_ = nullptr;
    }
}

void Window::FramebufferSizeCallback(WindowHandle window, int width, int height) {
    int h = std::max(1, height);
    int w = std::max(1, width);

    if (event_bus_) {
        event_bus_->Invoke<WindowResizeEvent>(static_cast<uint32_t>(w), static_cast<uint32_t>(h));
    }

    // Note: No glViewport — Filament handles viewport via View::setViewport
}

}  // namespace se
