#pragma once

#include <stdexcept>

namespace se {

class Renderer;
class InputManager;
class EventBus;
class SceneRenderer;

/**
 * ServiceLocator provides centralized access to engine services.
 * Uses Meyer's Singleton pattern for thread-safe lazy initialization.
 * 
 * Services are non-owning pointers - the Application owns the actual instances.
 * Services must be registered before use via Provide* methods.
 */
class ServiceLocator {
public:
    static ServiceLocator& Get() {
        static ServiceLocator instance;
        return instance;
    }

    // Service registration (non-owning pointers)
    void ProvideRenderer(Renderer* renderer) { renderer_ = renderer; }
    void ProvideInputManager(InputManager* input) { input_manager_ = input; }
    void ProvideEventBus(EventBus* eventBus) { event_bus_ = eventBus; }
    void ProvideSceneRenderer(SceneRenderer* sceneRenderer) { scene_renderer_ = sceneRenderer; }

    // Service access with validation
    Renderer& GetRenderer() const { 
        if (!renderer_) throw std::runtime_error("Renderer not registered with ServiceLocator");
        return *renderer_; 
    }
    
    InputManager& GetInputManager() const { 
        if (!input_manager_) throw std::runtime_error("InputManager not registered with ServiceLocator");
        return *input_manager_; 
    }
    
    EventBus& GetEventBus() const { 
        if (!event_bus_) throw std::runtime_error("EventBus not registered with ServiceLocator");
        return *event_bus_; 
    }
    
    SceneRenderer& GetSceneRenderer() const {
        if (!scene_renderer_) throw std::runtime_error("SceneRenderer not registered with ServiceLocator");
        return *scene_renderer_;
    }

    // Raw pointer access for optional checks
    Renderer* GetRendererPtr() const { return renderer_; }
    InputManager* GetInputManagerPtr() const { return input_manager_; }
    EventBus* GetEventBusPtr() const { return event_bus_; }
    SceneRenderer* GetSceneRendererPtr() const { return scene_renderer_; }

    // Availability checks
    bool HasRenderer() const { return renderer_ != nullptr; }
    bool HasInputManager() const { return input_manager_ != nullptr; }
    bool HasEventBus() const { return event_bus_ != nullptr; }
    bool HasSceneRenderer() const { return scene_renderer_ != nullptr; }

    // Reset all services (for shutdown/testing)
    void Reset() {
        renderer_ = nullptr;
        input_manager_ = nullptr;
        event_bus_ = nullptr;
        scene_renderer_ = nullptr;
    }

private:
    ServiceLocator() = default;
    ~ServiceLocator() = default;

    ServiceLocator(const ServiceLocator&) = delete;
    ServiceLocator& operator=(const ServiceLocator&) = delete;
    ServiceLocator(ServiceLocator&&) = delete;
    ServiceLocator& operator=(ServiceLocator&&) = delete;

    Renderer* renderer_ = nullptr;
    InputManager* input_manager_ = nullptr;
    EventBus* event_bus_ = nullptr;
    SceneRenderer* scene_renderer_ = nullptr;
};

}  // namespace se
