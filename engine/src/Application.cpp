#include "engine/Application.h"

#include <GLFW/glfw3.h>

#include <glm.hpp>

#include "Engine.h"
#include "engine/Log.h"
#include "engine/input/Input.h"

namespace se {
Application* Application::s_Instance = nullptr;

Application::Application(const ApplicationSpecification& specification) {
    if (s_Instance) {
        SE_LOG_ERROR("Application already exists!");
        return;
    }
    s_Instance = this;

#ifdef DEBUG
    LogInit(true);
#endif

    SE_LOG_INFO("Starting Simple Engine");

    WindowSpec windowSpec;
    windowSpec.Title      = specification.Name;
    windowSpec.Width      = specification.WindowWidth;
    windowSpec.Height     = specification.WindowHeight;
    windowSpec.Decorated  = specification.WindowDecorated;
    windowSpec.Fullscreen = specification.Fullscreen;
    windowSpec.VSync      = specification.VSync;
    windowSpec.IconPath   = specification.IconPath;

    // Create window
    window_ = std::unique_ptr<Window>(Window::Create(windowSpec));

    window_->Init();
    window_->SetEventCallback([this](Event& e) { OnEvent(e); });

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
    if (imguiLayer_) { imguiLayer_->OnDetach(); }

    // Cleanup layers
    for (auto& layer : layer_stack_) { layer->OnDetach(); }
    layer_stack_.clear();

    // Cleanup systems
    renderer_.reset();
    glfwTerminate();

    s_Instance = nullptr;
}

void Application::OnEvent(Event& event) {
    EventDispatcher dispatcher(event);
    dispatcher.Dispatch<WindowResizeEvent>([this](WindowResizeEvent& e) { return OnWindowResize(e); });
    dispatcher.Dispatch<WindowMinimizeEvent>([this](WindowMinimizeEvent& e) { return OnWindowMinimize(e); });
    dispatcher.Dispatch<WindowCloseEvent>([this](WindowCloseEvent& e) { return OnWindowClose(e); });

    for (auto it = layer_stack_.end(); it != layer_stack_.begin();) {
        (*--it)->OnEvent(event);
        if (event.Handled) break;
    }

    if (event.Handled) return;

    for (auto& eventCallback : event_callbacks_) {
        eventCallback(event);

        if (event.Handled) break;
    }
}

int Application::Run() {
    running_       = true;
    float lastTime = GetTime();

    SE_LOG_INFO("Application main loop started");

    while (running_) {
        // Check for window close
        if (Input::IsKeyPressed(Key::Escape)) { window_->RequestClose(); }

        if (window_->ShouldClose()) {
            Close();
            break;
        }

        // Calculate timestep
        float currentTime = GetTime();
        float timestep    = glm::clamp(currentTime - lastTime, 0.001f, 0.1f);
        lastTime          = currentTime;

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
        for (const std::unique_ptr<Layer>& layer : layer_stack_) { layer->OnUpdate(timestep); }

        // Render all layers
        for (const std::unique_ptr<Layer>& layer : layer_stack_) { layer->OnRender(); }

        // End frame
        renderer_->EndFrame();

        // ImGui rendering
        imguiLayer_->Begin();

        // Let layers draw their ImGui
        for (const std::unique_ptr<Layer>& layer : layer_stack_) { layer->OnImGuiRender(); }

        imguiLayer_->End();

        // Swap buffers and poll events
        window_->SwapBuffers();
        window_->OnUpdate();
    }

    SE_LOG_INFO("Application main loop ended");
    return 0;
}

void Application::Close() {
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
bool Application::OnWindowResize(WindowResizeEvent& e) {
    const uint32_t width = e.GetWidth(), height = e.GetHeight();
    if (width == 0 || height == 0) { return false; }

    auto& window = window_;

    // Renderer::Submit([&window, width, height]() mutable
    // {
    //     window->GetSwapChain().OnResize(width, height);
    // });

    return false;
}

bool Application::OnWindowMinimize(WindowMinimizeEvent& e) {
    minimized_ = e.IsMinimized();
    return false;
}

bool Application::OnWindowClose(WindowCloseEvent& e) {
    Close();
    return false;
}
}  // namespace se
