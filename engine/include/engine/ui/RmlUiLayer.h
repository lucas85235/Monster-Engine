#pragma once

#include "engine/Layer.h"
#include "engine/ui/RmlUiInterfaces.h"
#include "engine/events/Events.h"
#include <RmlUi/Core.h>

namespace se {

class EventBus;

class RmlUiLayer : public Layer {
   public:
    RmlUiLayer();
    ~RmlUiLayer();

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float ts) override;
    void OnRender() override;

    Rml::Context* GetContext() const { return context_; }

   private:
    RmlUiSystemInterface*     system_interface_ = nullptr;
    RmlUiRenderInterface*     render_interface_ = nullptr;
    RmlUiFontEngineInterface* font_interface_   = nullptr;
    Rml::Context*             context_          = nullptr;
    EventBus*                 event_bus_        = nullptr;
    
    void OnWindowResize(const WindowResizeEvent& e);
    void OnMouseMove(const MouseMovedEvent& e);
    void OnMouseButtonPressed(const MouseButtonPressedEvent& e);
    void OnMouseButtonReleased(const MouseButtonReleasedEvent& e);
    void OnMouseScrolled(const MouseScrolledEvent& e);
    void OnKeyPressed(const KeyPressedEvent& e);
    void OnKeyReleased(const KeyReleasedEvent& e);
    void OnKeyTyped(const KeyTypedEvent& e);
};

}  // namespace se
