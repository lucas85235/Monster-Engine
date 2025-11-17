#include "EventSampleLayer.h"

#include "engine/Application.h"
#include "engine/new_event_system/EventBus.h"
#include "events/Events.h"
EventSampleLayer::~EventSampleLayer() {}
void EventSampleLayer::OnDetach() {
    Layer::OnDetach();
}
void EventSampleLayer::OnAttach() {
    Layer::OnAttach();

    event_bus_ = &Application::Get().GetEventBus();
    event_bus_->AddListener<EnemySpawned>(SE_BIND_EVENT_FN(OnEnemySpawned));
    event_bus_->AddListener<NewWindowResizeEvent>(SE_BIND_EVENT_FN(OnWindowResizedNew));
    event_bus_->AddListener<NewWindowCloseEvent>(SE_BIND_EVENT_FN(OnWindowCloseNew));

    event_bus_->AddListener<SampleEventWithNoInputs>(SE_BIND_EVENT_FN(OnNoInputEventTriggered));
    event_bus_->AddListener<SampleEventWithOneInput>(SE_BIND_EVENT_FN(OnSingleInputEventTriggered));
    event_bus_->AddListener<SampleEventWithTwoInputs>(SE_BIND_EVENT_FN(OnTwoInputEventsTriggered));

    event_bus_->Invoke<SampleEventWithNoInputs>();
    event_bus_->Invoke<SampleEventWithOneInput>(42);
    event_bus_->Invoke<SampleEventWithTwoInputs>(42,54);
}
void EventSampleLayer::OnUpdate(float ts) {
    Layer::OnUpdate(ts);
}
void EventSampleLayer::OnRender() {
    Layer::OnRender();
}
void EventSampleLayer::OnImGuiRender() {
    Layer::OnImGuiRender();
}
void EventSampleLayer::OnEvent(Event& event) {
    Layer::OnEvent(event);


}

// system events
void EventSampleLayer::OnEnemySpawned(const EnemySpawned& e) {
    SE_LOG_INFO("Enemy spawned!");
}

void EventSampleLayer::OnWindowResizedNew(const NewWindowResizeEvent& e) {
    SE_LOG_INFO("Window was resized to: ({},{})", e.width, e.height);
}
void EventSampleLayer::OnWindowCloseNew(const NewWindowCloseEvent& e) {
    SE_LOG_INFO("Window should close");
}

// application specific events
void EventSampleLayer::OnNoInputEventTriggered(const SampleEventWithNoInputs& e) {
    SE_LOG_INFO("Event with no Input triggered!");
}

void EventSampleLayer::OnSingleInputEventTriggered(const SampleEventWithOneInput& e) {
    SE_LOG_INFO("Single Input event triggered! (first Input:{})",e.input);
}

void EventSampleLayer::OnTwoInputEventsTriggered(const SampleEventWithTwoInputs& e) {
    SE_LOG_INFO("Two Inputs event triggered! (first input:{}, second input:{})",e.input_1,e.input_2);
}
