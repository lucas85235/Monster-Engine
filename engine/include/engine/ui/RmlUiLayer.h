#pragma once

#include "engine/Layer.h"
#include "engine/ui/RmlUiInterfaces.h"
#include <RmlUi/Core.h>

namespace se {

class RmlUiLayer : public Layer {
   public:
    RmlUiLayer();
    ~RmlUiLayer();

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float ts) override;
    void OnRender() override;
    void OnEvent(Event& event) override;

    Rml::Context* GetContext() const { return context_; }

   private:
    RmlUiSystemInterface*     system_interface_ = nullptr;
    RmlUiRenderInterface*     render_interface_ = nullptr;
    RmlUiFontEngineInterface* font_interface_   = nullptr;
    Rml::Context*             context_          = nullptr;
    
    bool OnWindowResize(class WindowResizeEvent& e);
    bool OnMouseMove(class MouseMovedEvent& e);
    bool OnMouseButtonPressed(class MouseButtonPressedEvent& e);
    bool OnMouseButtonReleased(class MouseButtonReleasedEvent& e);
    bool OnMouseScrolled(class MouseScrolledEvent& e);
    bool OnKeyPressed(class KeyPressedEvent& e);
    bool OnKeyReleased(class KeyReleasedEvent& e);
    bool OnKeyTyped(class KeyTypedEvent& e);
};

}  // namespace se
