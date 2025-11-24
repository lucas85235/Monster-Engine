#include "engine/renderer/GraphicsContext.h"
#include "engine/Log.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>
#include <stdexcept>

namespace se {

GraphicsContext::GraphicsContext(GLFWwindow* windowHandle) : windowHandle_(windowHandle) {
    if (!windowHandle_) {
        throw std::runtime_error("Window handle is null!");
    }
}

GraphicsContext::~GraphicsContext() {}

void GraphicsContext::Init() {
    glfwMakeContextCurrent(windowHandle_);

    // Initialize GLAD
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        throw std::runtime_error("Failed to initialize GLAD");
    }

    SE_LOG_INFO("OpenGL Info:");
    SE_LOG_INFO("  Vendor: {}", (const char*)glGetString(GL_VENDOR));
    SE_LOG_INFO("  Renderer: {}", (const char*)glGetString(GL_RENDERER));
    SE_LOG_INFO("  Version: {}", (const char*)glGetString(GL_VERSION));
}

void GraphicsContext::SwapBuffers() {
    glfwSwapBuffers(windowHandle_);
}

} // namespace se