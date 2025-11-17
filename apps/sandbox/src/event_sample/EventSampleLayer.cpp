#include "EventSampleLayer.h"
EventSampleLayer::~EventSampleLayer() {}
void EventSampleLayer::OnDetach() {
    Layer::OnDetach();
}
void EventSampleLayer::OnAttach() {
    Layer::OnAttach();
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
