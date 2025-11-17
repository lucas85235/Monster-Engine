#include "EventSampleLayer.h"

#include "engine/new_event_system/EventManager.h"
EventSampleLayer::~EventSampleLayer() {}
void EventSampleLayer::OnDetach() {
    Layer::OnDetach();
}
void EventSampleLayer::OnAttach() {
    Layer::OnAttach();
    EventManager::Subscribe(EventType::SampleEvent, SE_BIND_EVENT_FN(OnEventSample));
}
void EventSampleLayer::OnUpdate(float ts) {
    Layer::OnUpdate(ts);
    NewEvent event;
    event.type = EventType::SampleEvent;

    EventManager::Publish(event);
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
void EventSampleLayer::OnEventSample(const NewEvent& event) {
    SE_LOG_INFO("Sample event was triggered!");
}
