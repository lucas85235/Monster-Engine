#include "engine/Application.h"

#include <GLFW/glfw3.h>

#include "Engine.h"
#include "engine/Log.h"
#include "engine/core/ServiceLocator.h"
#include "engine/core/Time.h"
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

    SE_LOG_INFO("Starting Monster Engine (Filament)");

    InputManager::Get().Init(event_bus_.get());

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

    // Create window (GLFW_NO_API — no OpenGL context)
    window_ = std::unique_ptr<Window>(Window::Create(windowSpec));
    window_->Init();

    // Create Filament context and renderer
    filament_context_ = std::make_unique<FilamentContext>();
    filament_context_->Init(
        window_->GetNativeWindow(),
        specification.WindowWidth,
        specification.WindowHeight,
        FilamentContext::Backend::Default  // Metal on macOS, Vulkan on Linux/Windows
    );

    filament_renderer_ = std::make_unique<FilamentRenderer>(*filament_context_);

    // Set default clear color (dark blue-gray)
    filament_renderer_->SetClearColor(0.1f, 0.1f, 0.15f, 1.0f);

    // Initialize rendering subsystems
    material_system_ = std::make_unique<MaterialSystem>();
    material_system_->Init(filament_context_->GetEngine());

    mesh_system_ = std::make_unique<MeshSystem>();
    mesh_system_->Init(filament_context_->GetEngine(), filament_context_->GetScene());

    light_system_ = std::make_unique<LightSystem>();
    light_system_->Init(filament_context_->GetEngine(), filament_context_->GetScene());

    // Register services with ServiceLocator
    ServiceLocator::Get().ProvideInputManager(&InputManager::Get());
    ServiceLocator::Get().ProvideEventBus(event_bus_.get());
    ServiceLocator::Get().ProvideFilamentContext(filament_context_.get());
    ServiceLocator::Get().ProvideFilamentRenderer(filament_renderer_.get());
    ServiceLocator::Get().ProvideMaterialSystem(material_system_.get());
    ServiceLocator::Get().ProvideMeshSystem(mesh_system_.get());
    ServiceLocator::Get().ProvideLightSystem(light_system_.get());

    // Register event listeners
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

    SE_LOG_INFO("Application initialized successfully (Filament renderer)");
}

Application::~Application() {
    SE_LOG_INFO("Shutting down Monster Engine");

    // Shutdown input manager
    InputManager::Get().Shutdown();

    // Cleanup layers
    for (auto& layer : layer_stack_) { layer->OnDetach(); }
    layer_stack_.clear();

    // Cleanup rendering subsystems (reverse init order)
    light_system_.reset();
    mesh_system_.reset();
    material_system_.reset();
    filament_renderer_.reset();
    filament_context_.reset();

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
        // Check for window close (Escape key)
        if (InputManager::Get().IsKeyDown(Key::Escape)) { window_->RequestClose(); }

        // Calculate timestep
        float currentTime = GetTime();
        float timestep    = std::min(currentTime - lastTime, 0.1f);
        if (timestep < 0.001f) timestep = 0.001f;
        lastTime          = currentTime;

        // Update global time
        Time::Update(timestep);

        // Update Input Manager
        InputManager::Get().Update();

        // Poll GLFW events
        window_->OnUpdate();

        // Dispatch events from the EventBus
        event_bus_->dispatch();

        // Skip rendering if minimized
        if (minimized_) continue;

        // Update framebuffer size
        int width, height;
        glfwGetFramebufferSize(window_->GetNativeWindow(), &width, &height);
        if (width > 0 && height > 0) {
            if (window_->GetWidth() != static_cast<uint32_t>(width) ||
                window_->GetHeight() != static_cast<uint32_t>(height)) {
                window_->SetWidth(width);
                window_->SetHeight(height);
                filament_context_->OnResize(width, height);
            }
        }

        // Begin Filament frame
        if (filament_renderer_->BeginFrame()) {
            // Update layers
            for (const std::unique_ptr<Layer>& layer : layer_stack_) { layer->OnUpdate(timestep); }

            // Render layers
            for (const std::unique_ptr<Layer>& layer : layer_stack_) { layer->OnRender(); }

            // End Filament frame (render + present)
            filament_renderer_->EndFrame();
        }

        // Apply FPS limiting if set
        window_->ApplyFrameRateLimit();

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
    filament_context_->OnResize(e.width, e.height);
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
