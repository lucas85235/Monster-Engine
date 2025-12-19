#pragma once
#include "apps/sandbox/src/SampleUtilities.h"
#include "engine/core/Layer.h"
#include "engine/events/EventBus.h"
#include "engine/events/Events.h"

using namespace se;

class InputSampleLayer : public se::Layer {
   public:
    InputSampleLayer();
    ~InputSampleLayer() override;
    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float ts) override;
    void OnRender() override;
    void OnImGuiRender() override;

    void UpdateCameraInput();

   private:
    void OnKeyPressed(const KeyPressedEvent& e);
    void OnMouseButtonPressed(const MouseButtonPressedEvent& e);

    Scope<Scene> scene_ = nullptr;
    EventBus* event_bus_ = nullptr;
};