#pragma once
#include "engine/Layer.h"
using namespace se;
class EventSampleLayer : public Layer {
   public:
    ~EventSampleLayer() override;
    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float ts) override;
    void OnRender() override;
    void OnImGuiRender() override;
    void OnEvent(Event& event) override;
};