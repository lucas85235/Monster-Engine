#pragma once

#include "engine/ImGuiLayer.h"
#include "engine/Layer.h"
#include "engine/Renderer.h"
#include "engine/Window.h"
#include <memory>
#include <vector>

namespace se {

class Application {
  public:
    Application(const ApplicationSpec& specification);
    ~Application();

    int Run();
    void Stop();

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
    std::unique_ptr<Window> window_;
    std::unique_ptr<Renderer> renderer_;
    std::shared_ptr<ImGuiLayer> imguiLayer_;

    std::vector<std::unique_ptr<Layer>> layer_stack_;
    bool running_ = false;

    static Application* s_Instance;
};

} // namespace se