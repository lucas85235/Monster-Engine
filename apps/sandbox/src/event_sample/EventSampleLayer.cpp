#include "EventSampleLayer.h"

#include "engine/core/Application.h"
#include "engine/events/EventBus.h"
#include "events/Events.h"

EventSampleLayer::~EventSampleLayer() {}

void EventSampleLayer::OnDetach() {}

void EventSampleLayer::OnAttach() {
    event_bus_ = &Application::Get().GetEventBus();
    
    // System events
    event_bus_->AddListener<EnemySpawned>(SE_BIND_EVENT_FN(OnEnemySpawned));
    event_bus_->AddListener<WindowResizeEvent>(SE_BIND_EVENT_FN(OnWindowResized));
    event_bus_->AddListener<WindowCloseEvent>(SE_BIND_EVENT_FN(OnWindowClose));

    // Custom sample events
    event_bus_->AddListener<SampleEventWithNoInputs>(SE_BIND_EVENT_FN(OnNoInputEventTriggered));
    event_bus_->AddListener<SampleEventWithOneInput>(SE_BIND_EVENT_FN(OnSingleInputEventTriggered));
    event_bus_->AddListener<SampleEventWithTwoInputs>(SE_BIND_EVENT_FN(OnTwoInputEventsTriggered));

    // Test invoking sample events
    event_bus_->Invoke<SampleEventWithNoInputs>();
    event_bus_->Invoke<SampleEventWithOneInput>(42);
    event_bus_->Invoke<SampleEventWithTwoInputs>(42, 54);
}

void EventSampleLayer::OnUpdate(float ts) {}

void EventSampleLayer::OnRender() {}

void EventSampleLayer::OnImGuiRender() {}

void EventSampleLayer::OnEnemySpawned(const EnemySpawned& e) {
    SE_LOG_INFO("Enemy spawned!");
}

void EventSampleLayer::OnWindowResized(const WindowResizeEvent& e) {
    SE_LOG_INFO("Window was resized to: ({},{})", e.width, e.height);
}

void EventSampleLayer::OnWindowClose(const WindowCloseEvent& e) {
    SE_LOG_INFO("Window should close");
}

void EventSampleLayer::OnNoInputEventTriggered(const SampleEventWithNoInputs& e) {
    SE_LOG_INFO("Event with no Input triggered!");
}

void EventSampleLayer::OnSingleInputEventTriggered(const SampleEventWithOneInput& e) {
    SE_LOG_INFO("Single Input event triggered! (first Input:{})", e.input);
}

void EventSampleLayer::OnTwoInputEventsTriggered(const SampleEventWithTwoInputs& e) {
    SE_LOG_INFO("Two Inputs event triggered! (first input:{}, second input:{})", e.input_1, e.input_2);
}
