#pragma once

#include <memory>
#include <vector>

#include "Engine.h"
#include "engine/ImGuiLayer.h"
#include "engine/Layer.h"
#include "engine/Window.h"
#include "engine/events/EventBus.h"
#include "engine/events/Events.h"
#include "engine/renderer/FilamentContext.h"
#include "engine/renderer/FilamentRenderer.h"
#include "engine/renderer/MaterialSystem.h"
#include "engine/renderer/MeshSystem.h"
#include "engine/renderer/LightSystem.h"
#include "engine/renderer/TextureSystem.h"
#include "engine/renderer/FilamentModelLoader.h"

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
    FilamentContext& GetFilamentContext() {
        return *filament_context_;
    }
    FilamentRenderer& GetFilamentRenderer() {
        return *filament_renderer_;
    }
    MaterialSystem& GetMaterialSystem() {
        return *material_system_;
    }
    MeshSystem& GetMeshSystem() {
        return *mesh_system_;
    }
    LightSystem& GetLightSystem() {
        return *light_system_;
    }
    TextureSystem& GetTextureSystem() {
        return *texture_system_;
    }
    FilamentModelLoader& GetModelLoader() {
        return *model_loader_;
    }
    EventBus& GetEventBus() {
        return *event_bus_;
    }

    // Active scene management
    void SetActiveScene(Scene* scene) {
        active_scene_ = scene;
    }
    Scene* GetActiveScene() {
        return active_scene_;
    }
    const Scene* GetActiveScene() const {
        return active_scene_;
    }

    static Application& Get();

    float GetTime();

   private:
    // Event handlers
    bool OnWindowResize(const WindowResizeEvent& e);
    bool OnWindowMinimize(const WindowMinimizeEvent& e);
    bool OnWindowClose(const WindowCloseEvent& e);

    std::unique_ptr<Window>           window_;
    std::unique_ptr<FilamentContext>   filament_context_;
    std::unique_ptr<FilamentRenderer> filament_renderer_;
    std::unique_ptr<MaterialSystem>   material_system_;
    std::unique_ptr<MeshSystem>       mesh_system_;
    std::unique_ptr<LightSystem>      light_system_;
    std::unique_ptr<TextureSystem>    texture_system_;
    std::unique_ptr<FilamentModelLoader> model_loader_;
    std::unique_ptr<ImGuiLayer>       imgui_layer_;

    ApplicationSpecification specification_;

    std::vector<std::unique_ptr<Layer>> layer_stack_;
    unsigned int                        layer_insert_index_ = 0;

    bool running_   = false;
    bool minimized_ = false;

    static Application* s_Instance;
    Scene*              active_scene_ = nullptr;

    std::unique_ptr<EventBus> event_bus_ = std::make_unique<EventBus>();
};

}  // namespace se
