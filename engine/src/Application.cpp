#include "engine/Application.h"

#include <GLFW/glfw3.h>

#include <glm.hpp>

#include "Engine.h"
#include "engine/Log.h"
#include "engine/core/ServiceLocator.h"
#include "engine/core/Time.h"
#include "engine/debug/DebugTools.h"
#include "engine/events/Events.h"
#include "engine/input/InputManager.h"

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
    windowSpec.Resizable  = specification.Resizable;
    windowSpec.IconPath   = specification.IconPath;
    windowSpec.event_bus   = event_bus_.get();

    // Create window
    window_ = std::unique_ptr<Window>(Window::Create(windowSpec));
    window_->Init();

    // Create and initialize renderer
    renderer_ = std::make_unique<Renderer>();
    renderer_->Init();

    // Register services with ServiceLocator
    ServiceLocator::Get().ProvideRenderer(renderer_.get());
    ServiceLocator::Get().ProvideInputManager(&InputManager::Get());
    ServiceLocator::Get().ProvideEventBus(event_bus_.get());

    // Set default clear color
    renderer_->SetClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    // Create and attach ImGui layer
    imguiLayer_ = std::make_shared<ImGuiLayer>();
    imguiLayer_->SetWindow(window_->GetNativeWindow());
    imguiLayer_->OnAttach();

    // Register event listeners with the new EventBus
    event_bus_->AddListener<WindowResizeEvent>(SE_BIND_EVENT_FN(OnWindowResize));
    event_bus_->AddListener<WindowMinimizeEvent>(SE_BIND_EVENT_FN(OnWindowMinimize));
    event_bus_->AddListener<WindowCloseEvent>(SE_BIND_EVENT_FN(OnWindowClose));

    // Forward input events to InputManager
    event_bus_->AddListener<KeyPressedEvent>(
        [](const KeyPressedEvent& e) { InputManager::Get().OnKeyPressed(e.keyCode); });
    event_bus_->AddListener<KeyReleasedEvent>(
        [](const KeyReleasedEvent& e) { InputManager::Get().OnKeyReleased(e.keyCode); });
    event_bus_->AddListener<MouseButtonPressedEvent>([](const MouseButtonPressedEvent& e) {
        InputManager::Get().OnMouseButtonPressed(e.button);
    });
    event_bus_->AddListener<MouseButtonReleasedEvent>([](const MouseButtonReleasedEvent& e) {
        InputManager::Get().OnMouseButtonReleased(e.button);
    });
    event_bus_->AddListener<MouseMovedEvent>(
        [](const MouseMovedEvent& e) { InputManager::Get().OnMouseMoved(e.x, e.y); });
    event_bus_->AddListener<MouseScrolledEvent>(
        [](const MouseScrolledEvent& e) { InputManager::Get().OnMouseScrolled(e.yOffset); });

    SE_LOG_INFO("Application initialized successfully");
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

    // Reset ServiceLocator
    ServiceLocator::Get().Reset();

    glfwTerminate();

    s_Instance = nullptr;
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

        // Update global time
        Time::Update(timestep);

        // Update Input Manager
        InputManager::Get().Update();

        // Poll events
        window_->OnUpdate();

        // Dispatch events from the EventBus
        event_bus_->dispatch();

        // Skip rendering if minimized
        if (minimized_) continue;
        
        // Begin debug profiling frame
        SE_DEBUG_TOOLS_FRAME_BEGIN();

        {
            SE_PROFILE_SCOPE("BeginFrame");
            renderer_->BeginFrame();
        }

        int width, height;
        glfwGetFramebufferSize(window_->GetNativeWindow(), &width, &height);

        {
            SE_PROFILE_SCOPE("Clear");
            renderer_->Clear();
        }

        if (window_->GetWidth() != static_cast<uint32_t>(width) ||
            window_->GetHeight() != static_cast<uint32_t>(height)) {
            window_->SetWidth(width);
            window_->SetHeight(height);
        }

        {
            SE_PROFILE_SCOPE("Layers Update");
            for (const std::unique_ptr<Layer>& layer : layer_stack_) { layer->OnUpdate(timestep); }
        }

        {
            SE_PROFILE_SCOPE("Layers Render");
            for (const std::unique_ptr<Layer>& layer : layer_stack_) { layer->OnRender(); }
        }

        {
            SE_PROFILE_SCOPE("EndFrame");
            renderer_->EndFrame();
        }

        // ImGui rendering
        {
            SE_PROFILE_SCOPE("ImGui");
            imguiLayer_->Begin();

            // Let layers draw their ImGui
            for (const std::unique_ptr<Layer>& layer : layer_stack_) { layer->OnImGuiRender(); }
            
            // Render debug tools (only in Debug builds)
            SE_DEBUG_TOOLS_RENDER();

            imguiLayer_->End();
        }
        
        // End debug profiling frame
        SE_DEBUG_TOOLS_FRAME_END();
        
        // Apply FPS limiting if set
        window_->ApplyFrameRateLimit();

        // Swap buffers
        window_->SwapBuffers();

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

bool Application::OnWindowResize(const WindowResizeEvent& e) {
    if (e.width == 0 || e.height == 0) {
        minimized_ = true;
        return false;
    }
    minimized_ = false;
    return false;
}

bool Application::OnWindowMinimize(const WindowMinimizeEvent& e) {
    minimized_ = e.minimized;
    return false;
}

bool Application::OnWindowClose(const WindowCloseEvent& e) {
    Close();
    return false;
}

}  // namespace se
