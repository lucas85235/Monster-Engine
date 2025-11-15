#include "engine/Window.h"

#include <GLFW/glfw3.h>

#include <stdexcept>

#include "engine/Input.h"
#include "engine/Log.h"
#include "engine/renderer/GraphicsContext.h"

namespace se {

static bool s_GLFWInitialized = false;

static void GLFWErrorCallback(int error, const char* description) {
    SE_LOG_ERROR("GLFW Error ({}): {}", error, description);
}

Window::Window(const ApplicationSpec& spec) : Window(spec.WindowWidth, spec.WindowHeight, spec.Name) {
    SetVSync(spec.VSync);
}

Window::Window(uint32_t width, uint32_t height, const std::string& title) : width_(width), height_(height), title_(title) {
    Init(width, height, title);
}

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

bool Window::ShouldClose() const {
    return glfwWindowShouldClose(handle_);
}

void Window::RequestClose() {
    glfwSetWindowShouldClose(handle_, GLFW_TRUE);
}

void Window::SwapBuffers() {
    context_->SwapBuffers();
}

void Window::Init(uint32_t width, uint32_t height, const std::string& title) {
    width_  = width;
    height_ = height;
    title_  = title;

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

    SE_LOG_INFO("Creating window {} ({}, {})", title_, width_, height_);

    handle_ = glfwCreateWindow((int)width_, (int)height_, title_.c_str(), nullptr, nullptr);
    if (!handle_) {
        glfwTerminate();
        throw std::runtime_error("Failed to create GLFW window");
    }

    // Create graphics context
    context_ = std::make_unique<GraphicsContext>(handle_);
    context_->Init();

    glfwSetWindowUserPointer(handle_, this);

    // Set callbacks
    glfwSetFramebufferSizeCallback(handle_, FramebufferSizeCallback);

    // Set input context
    Input::SetWindow(handle_);

    // Set initial viewport
    int fbWidth, fbHeight;
    glfwGetFramebufferSize(handle_, &fbWidth, &fbHeight);
    glViewport(0, 0, fbWidth, fbHeight);
}

void Window::Shutdown() {
    if (handle_) {
        glfwDestroyWindow(handle_);
        handle_ = nullptr;
    }
}

void Window::FramebufferSizeCallback(GLFWwindow* window, int width, int height) {
    int h = std::max(1, height);
    int w = std::max(1, width);

    SE_LOG_WARN("Window size callback: ({},{})", w, h);

    glViewport(0, 0, w, h);
}
}  // namespace se