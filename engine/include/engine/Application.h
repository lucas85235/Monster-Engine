#pragma once

#include <memory>
#include <vector>

#include "Engine.h"
#include "engine/ImGuiLayer.h"
#include "engine/Layer.h"
#include "engine/Renderer.h"
#include "engine/Window.h"
#include "event/ApplicationEvent.h"
#include "new_event_system/EventBus.h"
#include "new_event_system/NewApplicationEvents.h"

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
        layer_stack_.emplace(layer_stack_.begin() + layer_insert_index_, std::move(layer));
        layer_insert_index_++;
    }

    template <typename T>
    void PushOverlay() {
        static_assert(std::is_base_of<Layer, T>::value, "T must inherit from Layer");
        auto layer = std::make_unique<T>();
        layer->OnAttach();
        layer_stack_.emplace_back(std::move(layer));
    }

    Window& GetWindow() {
        return *window_;
    }
    Renderer& GetRenderer() {
        return *renderer_;
    }

    EventBus& GetEventBus() const {
        return *event_bus_;
    }

    static Application& Get();

    float GetTime();
    static bool  OnWindowResizeNew(const NewWindowResizeEvent& e);
    bool  OnWindowMinimizeNew(const NewWindowMinimizeEvent& e);
    bool  OnWindowCloseNew(const NewWindowCloseEvent& e);

   private:
    std::unique_ptr<Window>     window_;
    std::unique_ptr<Renderer>   renderer_;
    std::shared_ptr<ImGuiLayer> imguiLayer_;

    bool OnWindowResize(const WindowResizeEvent& e);
    bool OnWindowMinimize(const WindowMinimizeEvent& e);
    bool OnWindowClose(const WindowCloseEvent& e);

    ApplicationSpecification specification_;

    std::vector<std::unique_ptr<Layer>> layer_stack_;
    unsigned int                        layer_insert_index_ = 0;

    bool running_   = false;
    bool minimized_ = false;

    static Application* s_Instance;

    EventBus* event_bus_ = new EventBus();

    std::vector<EventCallbackFn> event_callbacks_;
};

}  // namespace se