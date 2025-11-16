#pragma once
#include "apps/sandbox/src/SampleUtilities.h"
#include "engine/Layer.h"
#include "engine/event/KeyEvent.h"
#include "engine/event/MouseEvent.h"

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
    void OnEvent(se::Event& event) override;
    bool OnKeyPressedEvent(KeyPressedEvent& e);
    bool OnMouseButtonPressed(MouseButtonPressedEvent& e);

    void UpdateCameraInput();

    private:
    Scope<Scene> scene_ = nullptr;
};