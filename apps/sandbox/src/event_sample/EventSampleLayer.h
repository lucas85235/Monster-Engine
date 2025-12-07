#pragma once
#include "engine/Layer.h"
#include "engine/events/EventBus.h"
#include "engine/events/Events.h"
#include "events/Events.h"

using namespace se;

struct EnemySpawned {
    std::string archetype;
    glm::vec3   position;
};

class EventSampleLayer : public Layer {
   public:
    ~EventSampleLayer() override;
    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float ts) override;
    void OnRender() override;
    void OnImGuiRender() override;

   private:
    void OnEnemySpawned(const EnemySpawned& e);
    void OnWindowResized(const WindowResizeEvent& e);
    void OnWindowClose(const WindowCloseEvent& e);
    void OnNoInputEventTriggered(const SampleEventWithNoInputs& e);
    void OnSingleInputEventTriggered(const SampleEventWithOneInput& e);
    void OnTwoInputEventsTriggered(const SampleEventWithTwoInputs& e);

    EventBus* event_bus_ = nullptr;
};