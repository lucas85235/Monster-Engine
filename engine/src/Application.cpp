#include "engine/Application.h"

#include <GLFW/glfw3.h>

#include <glm.hpp>

#include "Engine.h"
#include "engine/Log.h"
#include "engine/input/InputManager.h"
#include "engine/new_event_system/NewApplicationEvents.h"

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

    InputManager::Get().Init();

    WindowSpec windowSpec;
    windowSpec.Title      = specification.Name;
    windowSpec.Width      = specification.WindowWidth;
    windowSpec.Height     = specification.WindowHeight;
    windowSpec.Decorated  = specification.WindowDecorated;
    windowSpec.Fullscreen = specification.Fullscreen;
    windowSpec.VSync      = specification.VSync;
    windowSpec.IconPath   = specification.IconPath;
    windowSpec.EventBus   = event_bus_;

    // Create window
    window_ = std::unique_ptr<Window>(Window::Create(windowSpec));

    window_->Init();
    window_->SetEventCallback(SE_BIND_EVENT_FN(OnEvent));

    // Create and initialize renderer
    renderer_ = std::make_unique<Renderer>();
    renderer_->Init();

    // Set default clear color
    renderer_->SetClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    // Initialize the physics system
    physics_manager_ = CreateScope<PhysicsManager>();
    physics_manager_->Initialize();

    // Create and attach ImGui layer
    imguiLayer_ = std::make_shared<ImGuiLayer>();
    imguiLayer_->SetWindow(window_->GetNativeWindow());
    imguiLayer_->OnAttach();

    event_bus_->AddListener<NewWindowResizeEvent>(SE_BIND_EVENT_FN(OnWindowResizeNew));
    event_bus_->AddListener<NewWindowMinimizeEvent>(SE_BIND_EVENT_FN(OnWindowMinimizeNew));
    event_bus_->AddListener<NewWindowCloseEvent>(SE_BIND_EVENT_FN(OnWindowCloseNew));
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
    switch (event.GetEventType()) {
        case EventType::WindowResize:
            event_bus_->Invoke<NewWindowResizeEvent>();
            break;

        case EventType::WindowMinimize:
            event_bus_->Invoke<NewWindowMinimizeEvent>();
            break;

        case EventType::WindowClose:
            event_bus_->Invoke<NewWindowCloseEvent>();
            break;
        default:;
    }

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
        if (InputManager::Get().IsKeyDown(Key::Escape)) { window_->RequestClose(); }

        // Calculate timestep
        float currentTime = GetTime();
        float timestep    = glm::clamp(currentTime - lastTime, 0.001f, 0.1f);
        lastTime          = currentTime;

        // Update Input Manager
        InputManager::Get().Update();

        // Poll events
        window_->OnUpdate();

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

        // Swap buffers
        window_->SwapBuffers();

        event_bus_->dispatch();

        physics_manager_->Update(timestep);

        if (window_->ShouldClose()) {
            Close();
            break;
        }
    }

    SE_LOG_INFO("Application main loop ended");
    return 0;
}

void Application::Close() {
    running_ = false;
}

Application& Application::Get() {
    return *s_Instance;
}

float Application::GetTime() {
    return static_cast<float>(glfwGetTime());
}

// new event system //////////////////////////////////////////
bool Application::OnWindowResizeNew(const NewWindowResizeEvent& e) {
    const uint32_t width = e.width, height = e.height;
    if (width == 0 || height == 0) { return false; }
    return false;
}

bool Application::OnWindowMinimizeNew(const NewWindowMinimizeEvent& e) {
    minimized_ = e.minimized;
    return false;
}

bool Application::OnWindowCloseNew(const NewWindowCloseEvent& e) {
    Close();
    return false;
}

bool Application::OnWindowResize(const WindowResizeEvent& e) {
    const uint32_t width = e.GetWidth(), height = e.GetHeight();
    if (width == 0 || height == 0) { return false; }

    return false;
}

bool Application::OnWindowMinimize(const WindowMinimizeEvent& e) {
    minimized_ = e.IsMinimized();
    return false;
}

bool Application::OnWindowClose(const WindowCloseEvent& e) {
    Close();
    return false;
}
}  // namespace se
