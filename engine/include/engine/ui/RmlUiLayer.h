#pragma once

#include "engine/Layer.h"
#include "engine/ui/RmlUiInterfaces.h"
#include "engine/new_event_system/EventBus.h"
#include "engine/new_event_system/NewApplicationEvents.h"
#include "engine/new_event_system/InputEvents.h"

#include <RmlUi/Core.h>

namespace se {

class RmlUiLayer : public Layer {
   public:
    RmlUiLayer();
    virtual ~RmlUiLayer();

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(float ts) override;
    virtual void OnRender() override;

   private:
    RmlUiSystemInterface* system_interface_ = nullptr;
    RmlUiRenderInterface* render_interface_ = nullptr;
    Rml::Context* context_ = nullptr;

    // Event listener IDs for cleanup
    EventBus::ListenerId<NewWindowResizeEvent> resize_listener_id_;
    EventBus::ListenerId<NewMouseMovedEvent> mouse_moved_listener_id_;
    EventBus::ListenerId<NewMouseButtonPressedEvent> mouse_pressed_listener_id_;
    EventBus::ListenerId<NewMouseButtonReleasedEvent> mouse_released_listener_id_;
    EventBus::ListenerId<NewMouseScrolledEvent> mouse_scrolled_listener_id_;
    EventBus::ListenerId<NewKeyPressedEvent> key_pressed_listener_id_;
    EventBus::ListenerId<NewKeyReleasedEvent> key_released_listener_id_;
    EventBus::ListenerId<NewCharTypedEvent> char_typed_listener_id_;
};

}  // namespace se
