#pragma once
#include "engine/Layer.h"
#include "engine/new_event_system/EventBus.h"
#include "engine/new_event_system/NewApplicationEvents.h"
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
    void OnEvent(Event& event) override;

   private:
    void OnEnemySpawned(const EnemySpawned& e);
    void OnWindowResizedNew(const NewWindowResizeEvent& e);
    void OnWindowCloseNew(const NewWindowCloseEvent& e);
    void OnNoInputEventTriggered(const SampleEventWithNoInputs& e);
    void OnSingleInputEventTriggered(const SampleEventWithOneInput& e);
    void OnTwoInputEventsTriggered(const SampleEventWithTwoInputs& e);

    EventBus* event_bus_ = nullptr;
};