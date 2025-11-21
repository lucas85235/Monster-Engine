#include "engine/Window.h"

#include <GLFW/glfw3.h>
#include <stdexcept>

#include "engine/Application.h"
#include "engine/Log.h"
#include "engine/input/InputManager.h"
#include "engine/renderer/GraphicsContext.h"
#include "engine/new_event_system/InputEvents.h"

namespace se {

static bool s_GLFWInitialized = false;

static void GLFWErrorCallback(int error, const char* description) {
    SE_LOG_ERROR("GLFW Error ({}): {}", error, description);
}

Window::Window(const WindowSpec& spec) : spec_(spec), window_data_() {}

Window::~Window() {
    Shutdown();
}

void Window::OnUpdate() {
    glfwPollEvents();
}

void Window::SetVSync(bool enabled) {
    if (enabled)
        glfwSwapInterval(1);
    else
        glfwSwapInterval(0);

    vsync_ = enabled;
}
void Window::SetTitle(const std::string& title) {
    window_data_.Title = title;
    glfwSetWindowTitle(window_handle_, window_data_.Title.c_str());
}

bool Window::ShouldClose() const {
    return glfwWindowShouldClose(window_handle_);
}

void Window::RequestClose() const {
    event_bus_->Invoke<NewWindowCloseEvent>();
    glfwSetWindowShouldClose(window_handle_, GLFW_TRUE);
}

void Window::SwapBuffers() const {
    context_->SwapBuffers();
}

Window* Window::Create(const WindowSpec& specification) {
    return new Window(specification);
}

void Window::Init() {
    window_data_.Title  = spec_.Title;
    window_data_.Width  = spec_.Width;
    window_data_.Height = spec_.Height;
    event_bus_          = spec_.EventBus;

    if (!s_GLFWInitialized) {
        int success = glfwInit();
        if (!success) { throw std::runtime_error("Could not initialize GLFW!"); }
        glfwSetErrorCallback(GLFWErrorCallback);
        s_GLFWInitialized = true;
    }

    // Set OpenGL version hints
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    SE_LOG_INFO("Creating window {} ({}, {})", spec_.Title, spec_.Width, spec_.Height);

    window_handle_ = glfwCreateWindow(static_cast<int>(spec_.Width), static_cast<int>(spec_.Height), spec_.Title.c_str(), nullptr, nullptr);
    if (!window_handle_) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    // Create graphics context
    context_ = std::make_unique<GraphicsContext>(window_handle_);
    context_->Init();

    glfwSetWindowUserPointer(window_handle_, &window_data_);

    // Set callbacks
    glfwSetFramebufferSizeCallback(window_handle_, FramebufferSizeCallback);

    glfwSetKeyCallback(window_handle_, [](WindowHandle window, int key, int scancode, int action, int mods) {
        switch (action) {
            case GLFW_PRESS: {
                InputManager::Get().OnKeyPressed(static_cast<KeyCode>(key));
                event_bus_->Invoke<NewKeyPressedEvent>(static_cast<KeyCode>(key), scancode, mods);
                break;
            }
            case GLFW_RELEASE: {
                InputManager::Get().OnKeyReleased(static_cast<KeyCode>(key));
                event_bus_->Invoke<NewKeyReleasedEvent>(static_cast<KeyCode>(key), scancode, mods);
                break;
            }
            case GLFW_REPEAT: {
                // InputManager::Get().OnKeyRepeated(static_cast<KeyCode>(key));
                break;
            }
        }
    });

    glfwSetMouseButtonCallback(window_handle_, [](WindowHandle window, int button, int action, int mods) {
        switch (action) {
            case GLFW_PRESS: {
                InputManager::Get().OnMouseButtonPressed(static_cast<MouseButton>(button));
                event_bus_->Invoke<NewMouseButtonPressedEvent>(static_cast<MouseButton>(button));
                break;
            }
            case GLFW_RELEASE: {
                InputManager::Get().OnMouseButtonReleased(static_cast<MouseButton>(button));
                event_bus_->Invoke<NewMouseButtonReleasedEvent>(static_cast<MouseButton>(button));
                break;
            }
        }
    });

    glfwSetCursorPosCallback(window_handle_, [](WindowHandle window, double xpos, double ypos) {
        InputManager::Get().OnMouseMoved(static_cast<float>(xpos), static_cast<float>(ypos));
        event_bus_->Invoke<NewMouseMovedEvent>(static_cast<float>(xpos), static_cast<float>(ypos));
    });

    glfwSetScrollCallback(window_handle_, [](WindowHandle window, double xoffset, double yoffset) {
        event_bus_->Invoke<NewMouseScrolledEvent>(static_cast<float>(xoffset), static_cast<float>(yoffset));
    });

    glfwSetCharCallback(window_handle_, [](WindowHandle window, unsigned int codepoint) {
        event_bus_->Invoke<NewCharTypedEvent>(codepoint);
    });

    // Set initial viewport
    int frame_buffer_width, frame_buffer_height;
    glfwGetFramebufferSize(window_handle_, &frame_buffer_width, &frame_buffer_height);
    glViewport(0, 0, frame_buffer_width, frame_buffer_height);
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


    // verify later if this logic is correct
    event_bus_->Invoke<NewWindowResizeEvent>(h,w);

    glViewport(0, 0, w, h);
}
}  // namespace se