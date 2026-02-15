#include "engine/Application.h"

#include <chrono>
#include <GLFW/glfw3.h>

#include "Engine.h"
#include "engine/Log.h"
#include "engine/core/ServiceLocator.h"
#include "engine/core/Time.h"
#include "engine/core/PerformanceProfiler.h"
#include "engine/console/ConsoleSystem.h"
#include "engine/console/DeveloperConsoleLayer.h"
#include "engine/events/Events.h"
#include "engine/input/InputManager.h"
#include "engine/ui/native/NativeUiLayer.h"
#include "engine/ui/native/NativeUiRenderer.h"
#include "engine/ui/native/retained/RetainedUi.h"

namespace se {
Application* Application::s_Instance = nullptr;

Application::Application(const ApplicationSpecification& specification) {
    if (s_Instance) {
        SE_LOG_ERROR("Application already exists!");
        return;
    }
    s_Instance = this;
    specification_ = specification;

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

    if (specification.EnableImGui) {
        SE_LOG_WARN("EnableImGui is ignored in Filament-first runtime; using native UI overlays.");
    }

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
    render_settings_system_ = std::make_unique<RenderSettingsSystem>();
    render_settings_system_->Init(filament_renderer_.get());

    // Initialize rendering subsystems
    material_system_ = std::make_unique<MaterialSystem>();
    material_system_->Init(filament_context_->GetEngine());

    mesh_system_ = std::make_unique<MeshSystem>();
    mesh_system_->Init(filament_context_->GetEngine(), filament_context_->GetScene());

    light_system_ = std::make_unique<LightSystem>();
    light_system_->Init(filament_context_->GetEngine(), filament_context_->GetScene());

    texture_system_ = std::make_unique<TextureSystem>();
    texture_system_->Init(filament_context_->GetEngine());

    model_loader_ = std::make_unique<FilamentModelLoader>();
    model_loader_->Init(filament_context_->GetEngine(), filament_context_->GetScene());

    // Register services with ServiceLocator
    ServiceLocator::Get().ProvideInputManager(&InputManager::Get());
    ServiceLocator::Get().ProvideEventBus(event_bus_.get());
    ServiceLocator::Get().ProvideFilamentContext(filament_context_.get());
    ServiceLocator::Get().ProvideFilamentRenderer(filament_renderer_.get());
    ServiceLocator::Get().ProvideMaterialSystem(material_system_.get());
    ServiceLocator::Get().ProvideMeshSystem(mesh_system_.get());
    ServiceLocator::Get().ProvideLightSystem(light_system_.get());
    ServiceLocator::Get().ProvideTextureSystem(texture_system_.get());
    ServiceLocator::Get().ProvideModelLoader(model_loader_.get());
    ServiceLocator::Get().ProvideRenderSettingsSystem(render_settings_system_.get());

    // Init receives window size (screen points) for UI layout.
    // The framebuffer size will be corrected on the first OnResize from the main loop.
    {
        int winW = static_cast<int>(specification.WindowWidth);
        int winH = static_cast<int>(specification.WindowHeight);
        glfwGetWindowSize(window_->GetNativeWindow(), &winW, &winH);
        ui::NativeUiRenderer::Get().Init(filament_context_.get(), filament_renderer_.get(),
                                         static_cast<uint32_t>(winW), static_cast<uint32_t>(winH));
        ui::retained::RetainedUiContext::Get().Init();
        ui::retained::RetainedUiContext::Get().SetViewport(static_cast<float>(winW),
                                                           static_cast<float>(winH));
    }

    ConsoleSystem::Get().Init();
    ServiceLocator::Get().ProvideConsoleSystem(&ConsoleSystem::Get());
    PushOverlay<ui::NativeUiLayer>();
    PushOverlay<DeveloperConsoleLayer>();

    // Register event listeners
    event_bus_->AddListener<WindowResizeEvent>(SE_BIND_EVENT_FN(OnWindowResize));
    event_bus_->AddListener<WindowMinimizeEvent>(SE_BIND_EVENT_FN(OnWindowMinimize));
    event_bus_->AddListener<WindowCloseEvent>(SE_BIND_EVENT_FN(OnWindowClose));
    event_bus_->AddListener<WindowFocusEvent>(
        [](const WindowFocusEvent& e) { InputManager::Get().OnWindowFocusChanged(e.focused); });

    // Forward input events to InputManager
    event_bus_->AddListener<KeyPressedEvent>(
        [](const KeyPressedEvent& e) { InputManager::Get().OnKeyPressed(e.keyCode, e.IsRepeat()); });
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
    event_bus_->AddListener<TextInputEvent>(
        [](const TextInputEvent& e) { InputManager::Get().OnTextInput(e.codepoint); });

    SE_LOG_INFO("Application initialized successfully (Filament renderer)");
}

Application::~Application() {
    SE_LOG_INFO("Shutting down Monster Engine");

    // Shutdown console before input, because it persists bindings through InputManager.
    ConsoleSystem::Get().Shutdown();

    // Shutdown input manager
    InputManager::Get().Shutdown();

    // Cleanup layers
    for (auto& layer : layer_stack_) { layer->OnDetach(); }
    layer_stack_.clear();

    ui::NativeUiRenderer::Get().Shutdown();
    ui::retained::RetainedUiContext::Get().Shutdown();

    // Reset service pointers before tearing down subsystem instances to avoid
    // stale pointer access during late object destruction.
    ServiceLocator::Get().Reset();

    // Cleanup rendering subsystems (reverse init order)
    model_loader_.reset();
    if (render_settings_system_) {
        render_settings_system_->Shutdown();
    }
    render_settings_system_.reset();
    texture_system_.reset();
    light_system_.reset();
    mesh_system_.reset();
    material_system_.reset();
    filament_renderer_.reset();
    filament_context_.reset();

    glfwTerminate();

    s_Instance = nullptr;
}

int Application::Run() {
    running_       = true;
    float lastTime = GetTime();

    using Clock = std::chrono::high_resolution_clock;
    auto toMs = [](const Clock::time_point& start, const Clock::time_point& end) -> float {
        return std::chrono::duration<float, std::milli>(end - start).count();
    };

    SE_LOG_INFO("Application main loop started");

    while (running_) {
        auto& perf = PerformanceProfiler::Get();
        perf.BeginFrame();
        PerformanceProfiler::FrameSectionTimes sections{};

        // Check for window close (Escape key)
        if (!ConsoleSystem::Get().IsVisible() && InputManager::Get().IsKeyDown(Key::Escape)) {
            window_->RequestClose();
        }

        // Calculate timestep
        float currentTime = GetTime();
        float timestep    = std::min(currentTime - lastTime, 0.1f);
        if (timestep < 0.001f) timestep = 0.001f;
        lastTime          = currentTime;

        // Update global time
        Time::Update(timestep);

        // Update Input Manager
        auto inputStart = Clock::now();
        InputManager::Get().Update();
        sections.inputUpdateTimeMs = toMs(inputStart, Clock::now());

        // Poll GLFW events
        auto eventStart = Clock::now();
        window_->OnUpdate();

        // Dispatch events from the EventBus
        event_bus_->dispatch();
        sections.eventPumpTimeMs = toMs(eventStart, Clock::now());

        // Update gameplay/UI layers even if Filament skips a frame.
        // This avoids losing edge-triggered input (mouse/key clicks) under low FPS.
        auto layerUpdateStart = Clock::now();
        for (const std::unique_ptr<Layer>& layer : layer_stack_) {
            const bool suppressInput =
                ConsoleSystem::Get().IsVisible() && layer->GetName() != "DeveloperConsoleLayer";
            InputManager::Get().SetInputSuppressed(suppressInput);
            layer->OnUpdate(timestep);
        }
        InputManager::Get().SetInputSuppressed(false);
        sections.layerUpdateTimeMs = toMs(layerUpdateStart, Clock::now());

        // Advance glTF skeletal animation clocks independently from rendering.
        auto animStart = Clock::now();
        if (model_loader_) {
            model_loader_->UpdateAnimations(timestep);
        }
        sections.animationTimeMs = toMs(animStart, Clock::now());

        if (!minimized_) {
            auto settingsStart = Clock::now();
            if (render_settings_system_ && render_settings_system_->IsDirty()) {
                render_settings_system_->Apply();
            }
            sections.settingsApplyTimeMs = toMs(settingsStart, Clock::now());

            // Update framebuffer size
            auto resizeStart = Clock::now();
            int fbWidth, fbHeight;
            glfwGetFramebufferSize(window_->GetNativeWindow(), &fbWidth, &fbHeight);
            int winWidth, winHeight;
            glfwGetWindowSize(window_->GetNativeWindow(), &winWidth, &winHeight);
            if (fbWidth > 0 && fbHeight > 0 && winWidth > 0 && winHeight > 0) {
                if (window_->GetWidth() != static_cast<uint32_t>(fbWidth) ||
                    window_->GetHeight() != static_cast<uint32_t>(fbHeight)) {
                    window_->SetWidth(fbWidth);
                    window_->SetHeight(fbHeight);
                    filament_context_->OnResize(fbWidth, fbHeight);
                    ui::NativeUiRenderer::Get().OnResize(
                        static_cast<uint32_t>(fbWidth), static_cast<uint32_t>(fbHeight),
                        static_cast<uint32_t>(winWidth), static_cast<uint32_t>(winHeight));
                    ui::retained::RetainedUiContext::Get().SetViewport(
                        static_cast<float>(winWidth), static_cast<float>(winHeight));
                }
            }
            sections.resizeHandlingTimeMs = toMs(resizeStart, Clock::now());

            // Begin Filament frame
            if (filament_renderer_->BeginFrame()) {
                auto renderSetupStart = Clock::now();
                ui::NativeUiRenderer::Get().BeginFrame();
                sections.renderSetupTimeMs = toMs(renderSetupStart, Clock::now());

                // Render layers
                auto layerRenderStart = Clock::now();
                for (const std::unique_ptr<Layer>& layer : layer_stack_) {
                    const bool suppressInput =
                        ConsoleSystem::Get().IsVisible() && layer->GetName() != "DeveloperConsoleLayer";
                    InputManager::Get().SetInputSuppressed(suppressInput);
                    layer->OnRender();
                }
                InputManager::Get().SetInputSuppressed(false);
                sections.layerRenderTimeMs = toMs(layerRenderStart, Clock::now());

                auto uiEndStart = Clock::now();
                ui::NativeUiRenderer::Get().EndFrame();
                sections.uiEndFrameTimeMs = toMs(uiEndStart, Clock::now());

                // End Filament frame (render + present)
                auto presentStart = Clock::now();
                filament_renderer_->EndFrame();
                sections.presentTimeMs = toMs(presentStart, Clock::now());
            }
        }

        // Apply FPS limiting if set
        auto limiterStart = Clock::now();
        window_->ApplyFrameRateLimit();
        sections.frameLimiterTimeMs = toMs(limiterStart, Clock::now());

        perf.SetFrameSectionTimes(sections);
        perf.SetUpdateTime(sections.inputUpdateTimeMs + sections.eventPumpTimeMs +
                           sections.settingsApplyTimeMs + sections.resizeHandlingTimeMs);
        perf.SetRenderTime(sections.renderSetupTimeMs + sections.layerUpdateTimeMs +
                           sections.animationTimeMs + sections.layerRenderTimeMs +
                           sections.uiEndFrameTimeMs + sections.presentTimeMs);
        perf.EndFrame();

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

    // e.width/height are framebuffer pixels (from glfwSetFramebufferSizeCallback).
    // Retrieve window size in screen points for UI layout.
    int winW = static_cast<int>(e.width);
    int winH = static_cast<int>(e.height);
    if (window_) {
        glfwGetWindowSize(window_->GetNativeWindow(), &winW, &winH);
    }
    ui::NativeUiRenderer::Get().OnResize(e.width, e.height,
                                         static_cast<uint32_t>(winW),
                                         static_cast<uint32_t>(winH));
    ui::retained::RetainedUiContext::Get().SetViewport(static_cast<float>(winW),
                                                       static_cast<float>(winH));
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
