
#include "engine/Application.h"
#include "engine/Input.h"
#include "engine/Log.h"
#include <GLFW/glfw3.h>
#include <glm.hpp>
#include <imgui.h>

namespace se {

Application* Application::s_Instance = nullptr;

Application::Application(const ApplicationSpec& specification) {
    if (s_Instance) {
        SE_LOG_ERROR("Application already exists!");
        return;
    }
    s_Instance = this;

    SE_LOG_INFO("Starting Simple Engine");

    // Create window
    window_ = std::make_unique<Window>(specification);

    // Create and initialize renderer
    renderer_ = std::make_unique<Renderer>();
    renderer_->Init();

    // Set default clear color
    renderer_->SetClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    // Create and attach ImGui layer
    imguiLayer_ = std::make_shared<ImGuiLayer>();
    imguiLayer_->SetWindow(window_->GetNativeWindow());
    imguiLayer_->OnAttach();
}

Application::~Application() {
    SE_LOG_INFO("Shutting down Simple Engine");

    // Detach ImGui
    if (imguiLayer_) {
        imguiLayer_->OnDetach();
    }

    // Cleanup layers
    for (auto& layer : layer_stack_) {
        layer->OnDetach();
    }
    layer_stack_.clear();

    // Cleanup systems
    renderer_.reset();
    window_.reset();
    glfwTerminate();

    s_Instance = nullptr;
}

int Application::Run() {
    running_ = true;
    float lastTime = GetTime();

    SE_LOG_INFO("Application main loop started");

    while (running_) {
        // Check for window close
        if (Input::IsKeyPressed(GLFW_KEY_ESCAPE)) {
            window_->RequestClose();
        }

        if (window_->ShouldClose()) {
            Stop();
            break;
        }

        // Calculate timestep
        float currentTime = GetTime();
        float timestep = glm::clamp(currentTime - lastTime, 0.001f, 0.1f);
        lastTime = currentTime;

        // Begin frame
        renderer_->BeginFrame();

        int width, height;
        glfwGetFramebufferSize(window_->GetNativeWindow(), &width, &height);

        // Clear screen with the configured color
        renderer_->Clear();

        if (window_->GetWidth() != width || window_->GetHeight() != height) {
            window_->SetWidth(width);
            window_->SetHeight(height);
        }

        // Update all layers
        for (const std::unique_ptr<Layer>& layer : layer_stack_) {
            layer->OnUpdate(timestep);
        }

        // Render all layers
        for (const std::unique_ptr<Layer>& layer : layer_stack_) {
            layer->OnRender();
        }

        // End frame
        renderer_->EndFrame();

        // ImGui rendering
        imguiLayer_->Begin();

        // Let layers draw their ImGui
        for (const std::unique_ptr<Layer>& layer : layer_stack_) {
            layer->OnImGuiRender();
        }

        imguiLayer_->End();

        // Swap buffers and poll events
        window_->SwapBuffers();
        window_->OnUpdate();
    }

    SE_LOG_INFO("Application main loop ended");
    return 0;
}

void Application::Stop() {
    running_ = false;
}

void Application::PushOverlay() {
    // TODO: Implement overlay support
}

Application& Application::Get() {
    return *s_Instance;
}

float Application::GetTime() {
    return static_cast<float>(glfwGetTime());
}

} // namespace se