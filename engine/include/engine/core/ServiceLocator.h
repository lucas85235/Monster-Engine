#pragma once

#include <stdexcept>

namespace se {

class Renderer;
class InputManager;
class EventBus;

class ServiceLocator {
public:
    static void Initialize() {
        instance_ = new ServiceLocator();
    }

    static void Shutdown() {
        delete instance_;
        instance_ = nullptr;
    }

    static ServiceLocator& Get() {
        if (!instance_) {
            throw std::runtime_error("ServiceLocator not initialized");
        }
        return *instance_;
    }

    void ProvideRenderer(Renderer* renderer) { renderer_ = renderer; }
    void ProvideInputManager(InputManager* input) { input_manager_ = input; }
    void ProvideEventBus(EventBus* eventBus) { event_bus_ = eventBus; }

    Renderer* GetRenderer() const { return renderer_; }
    InputManager* GetInputManager() const { return input_manager_; }
    EventBus* GetEventBus() const { return event_bus_; }

    bool HasRenderer() const { return renderer_ != nullptr; }
    bool HasInputManager() const { return input_manager_ != nullptr; }
    bool HasEventBus() const { return event_bus_ != nullptr; }

private:
    ServiceLocator() = default;
    ~ServiceLocator() = default;

    ServiceLocator(const ServiceLocator&) = delete;
    ServiceLocator& operator=(const ServiceLocator&) = delete;

    static ServiceLocator* instance_;

    Renderer* renderer_ = nullptr;
    InputManager* input_manager_ = nullptr;
    EventBus* event_bus_ = nullptr;
};

}  // namespace se
