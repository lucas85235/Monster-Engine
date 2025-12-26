#include "engine/ui/RmlUiLayer.h"

#include <GLFW/glfw3.h>
#include <RmlUi/Debugger.h>

#include "engine/core/Application.h"
#include "engine/core/Log.h"
#include "engine/events/EventBus.h"
#include "engine/events/Events.h"

namespace se {

RmlUiLayer::RmlUiLayer() : Layer("RmlUiLayer") {}

RmlUiLayer::~RmlUiLayer() {}

void RmlUiLayer::OnAttach() {
    system_interface_ = new RmlUiSystemInterface();
    render_interface_ = new RmlUiRenderInterface();
    font_interface_   = new RmlUiFontEngineInterface();

    Rml::SetSystemInterface(system_interface_);
    Rml::SetRenderInterface(render_interface_);
    Rml::SetFontEngineInterface(font_interface_);

    if (!Rml::Initialise()) {
        SE_LOG_ERROR("Failed to initialise RmlUi");
        return;
    }

    auto& window = Application::Get().GetWindow();
    int   width  = window.GetWidth();
    int   height = window.GetHeight();

    render_interface_->SetViewport(width, height);

    context_ = Rml::CreateContext("main", Rml::Vector2i(width, height));
    if (!context_) {
        SE_LOG_ERROR("Failed to create RmlUi context");
        return;
    }

    Rml::Debugger::Initialise(context_);

    // Subscribe to events via EventBus
    event_bus_ = &Application::Get().GetEventBus();
    event_bus_->AddListener<WindowResizeEvent>(SE_BIND_EVENT_FN(OnWindowResize));
    event_bus_->AddListener<MouseMovedEvent>(SE_BIND_EVENT_FN(OnMouseMove));
    event_bus_->AddListener<MouseButtonPressedEvent>(SE_BIND_EVENT_FN(OnMouseButtonPressed));
    event_bus_->AddListener<MouseButtonReleasedEvent>(SE_BIND_EVENT_FN(OnMouseButtonReleased));
    event_bus_->AddListener<MouseScrolledEvent>(SE_BIND_EVENT_FN(OnMouseScrolled));
    event_bus_->AddListener<KeyPressedEvent>(SE_BIND_EVENT_FN(OnKeyPressed));
    event_bus_->AddListener<KeyReleasedEvent>(SE_BIND_EVENT_FN(OnKeyReleased));
    event_bus_->AddListener<KeyTypedEvent>(SE_BIND_EVENT_FN(OnKeyTyped));
}

void RmlUiLayer::OnDetach() {
    Rml::Shutdown();
    delete system_interface_;
    delete render_interface_;
    delete font_interface_;
}

void RmlUiLayer::OnUpdate(float ts) {
    if (context_) context_->Update();
}

void RmlUiLayer::OnRender() {
    if (context_) {
        // Setup render state for RmlUi
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_DEPTH_TEST);

        context_->Render();

        glEnable(GL_DEPTH_TEST);
        glDisable(GL_BLEND);
    }
}

void RmlUiLayer::OnWindowResize(const WindowResizeEvent& e) {
    if (context_) {
        context_->SetDimensions(Rml::Vector2i(e.width, e.height));
        render_interface_->SetViewport(e.width, e.height);
    }
}

void RmlUiLayer::OnMouseMove(const MouseMovedEvent& e) {
    if (context_) context_->ProcessMouseMove(static_cast<int>(e.x), static_cast<int>(e.y), 0);
}

void RmlUiLayer::OnMouseButtonPressed(const MouseButtonPressedEvent& e) {
    if (context_) context_->ProcessMouseButtonDown(e.button, 0);
}

void RmlUiLayer::OnMouseButtonReleased(const MouseButtonReleasedEvent& e) {
    if (context_) context_->ProcessMouseButtonUp(e.button, 0);
}

void RmlUiLayer::OnMouseScrolled(const MouseScrolledEvent& e) {
    if (context_) context_->ProcessMouseWheel(-e.yOffset, 0);
}

void RmlUiLayer::OnKeyPressed(const KeyPressedEvent& e) {
    if (context_) {
        // Map GLFW keys to RmlUi keys (minimal for now)
    }
}

void RmlUiLayer::OnKeyReleased(const KeyReleasedEvent& e) {
    if (context_) {
        // context_->ProcessKeyUp(key, 0);
    }
}

void RmlUiLayer::OnKeyTyped(const KeyTypedEvent& e) {
    if (context_) {
        if (e.keyCode >= 32)  // Printable
            context_->ProcessTextInput((Rml::Character)e.keyCode);
    }
}

}  // namespace se
