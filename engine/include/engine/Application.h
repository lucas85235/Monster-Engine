#pragma once

#include <memory>
#include <vector>

#include "Engine.h"
#include "engine/ImGuiLayer.h"
#include "engine/Layer.h"
#include "engine/Renderer.h"
#include "engine/Window.h"
#include "event/ApplicationEvent.h"

namespace se {

struct ApplicationSpecification {
    std::string           Name        = "Simple-Engine";
    uint32_t              WindowWidth = 800, WindowHeight = 600;
    bool                  WindowDecorated = false;
    bool                  Fullscreen      = false;
    bool                  VSync           = true;
    std::string           WorkingDirectory;
    bool                  StartMaximized = true;
    bool                  Resizable      = true;
    bool                  EnableImGui    = true;
    std::filesystem::path IconPath;
};

class Application {
   public:
    Application(const ApplicationSpecification& specification);
    ~Application();
    void OnEvent(Event& event);

    int  Run();
    void Close();

    template <typename T>
    void PushLayer() {
        static_assert(std::is_base_of<Layer, T>::value, "T must inherit from Layer");
        auto layer = std::make_unique<T>();
        layer->OnAttach();
        layer_stack_.push_back(std::move(layer));
    }

    void PushOverlay();

    Window& GetWindow() {
        return *window_;
    }
    Renderer& GetRenderer() {
        return *renderer_;
    }

    static Application& Get();

    float GetTime();

   private:
    std::unique_ptr<Window>     window_;
    std::unique_ptr<Renderer>   renderer_;
    std::shared_ptr<ImGuiLayer> imguiLayer_;

    bool OnWindowResize(WindowResizeEvent& e);
    bool OnWindowMinimize(WindowMinimizeEvent& e);
    bool OnWindowClose(WindowCloseEvent& e);

    ApplicationSpecification specification_;

    std::vector<std::unique_ptr<Layer>> layer_stack_;

    bool running_   = false;
    bool minimized_ = false;

    static Application* s_Instance;

    std::vector<EventCallbackFn> event_callbacks_;
};

}  // namespace se