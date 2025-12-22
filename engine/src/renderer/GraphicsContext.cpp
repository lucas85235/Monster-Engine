#include "engine/renderer/GraphicsContext.h"

#include <GLFW/glfw3.h>
#include <glad/glad.h>

#include <stdexcept>

#include "engine/core/Log.h"
#include "engine/rhi/vulkan/vulkan_device.h"
#include "engine/utils/GLUtils.h"

namespace se {

GraphicsContext::GraphicsContext(GLFWwindow* windowHandle, RHI::API api) : windowHandle_(windowHandle), api_(api) {
    if (!windowHandle_) { throw std::runtime_error("Window handle is null!"); }
}

GraphicsContext::~GraphicsContext() {
    if (device_) { device_->Shutdown(); }
}

void GraphicsContext::Init() {
    SE_LOG_INFO("GraphicsContext::Init() - Starting (API: {})", api_ == RHI::API::OpenGL ? "OpenGL" : "Vulkan");
    
    if (api_ == RHI::API::OpenGL) {
        SE_LOG_INFO("GraphicsContext::Init() - Making context current");
        glfwMakeContextCurrent(windowHandle_);

        // Initialize GLAD
        SE_LOG_INFO("GraphicsContext::Init() - Loading GLAD");
        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) { throw std::runtime_error("Failed to initialize GLAD"); }

        SE_LOG_INFO("OpenGL Info:");
        SE_LOG_INFO("  Vendor: {}", (const char*)glGetString(GL_VENDOR));
        SE_LOG_INFO("  Renderer: {}", (const char*)glGetString(GL_RENDERER));
        SE_LOG_INFO("  Version: {}", (const char*)glGetString(GL_VERSION));

        // Initialize OpenGL Debug Output
        SE_LOG_INFO("GraphicsContext::Init() - Initializing OpenGL debug callback");
        Renderer::Utils::InitOpenGLDebugMessageCallback();
        SE_LOG_INFO("GraphicsContext::Init() - OpenGL debug callback initialized");
    }

    // Create RHI Device
    SE_LOG_INFO("GraphicsContext::Init() - Creating RHI Device");
    device_ = RHI::DeviceFactory::Create(api_);
    if (!device_) { throw std::runtime_error("Failed to create RHI device"); }
    SE_LOG_INFO("GraphicsContext::Init() - RHI Device created");

    // RHI requires window for Vulkan
    if (api_ == RHI::API::Vulkan) {
        SE_LOG_INFO("GraphicsContext::Init() - Setting Vulkan window");
        static_cast<RHI::VulkanDevice*>(device_.get())->SetWindow(windowHandle_);
    }

    SE_LOG_INFO("GraphicsContext::Init() - Initializing RHI Device");
    if (!device_->Initialize()) { throw std::runtime_error("Failed to initialize RHI device"); }
    SE_LOG_INFO("GraphicsContext::Init() - RHI Device initialized successfully");
}

void GraphicsContext::SwapBuffers() {
    if (api_ == RHI::API::OpenGL) {
        glfwSwapBuffers(windowHandle_);
    } else {
        // For Vulkan, EndFrame handles presentation
        // But we need to ensure BeginFrame was called if we abuse SwapBuffers for this.
        // For now, assuming new renderer loop calls BeginFrame/EndFrame explicitly.
        // This method might be deprecated for Vulkan path.
    }
}

}  // namespace se