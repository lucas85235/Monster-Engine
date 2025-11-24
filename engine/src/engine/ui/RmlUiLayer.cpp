#include "engine/ui/RmlUiLayer.h"
#include <RmlUi/Debugger.h>

#include "engine/Application.h"
#include "engine/Log.h"
#include "engine/event/ApplicationEvent.h"
#include "engine/event/KeyEvent.h"
#include "engine/event/MouseEvent.h"

#include <GLFW/glfw3.h>

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
    int width = window.GetWidth();
    int height = window.GetHeight();

    render_interface_->SetViewport(width, height);

    context_ = Rml::CreateContext("main", Rml::Vector2i(width, height));
    if (!context_) {
        SE_LOG_ERROR("Failed to create RmlUi context");
        return;
    }

    Rml::Debugger::Initialise(context_);
    
    // Load fonts?
    // Rml::LoadFontFace("assets/fonts/Lato-Regular.ttf");
}

void RmlUiLayer::OnDetach() {
    Rml::Shutdown();
    delete system_interface_;
    delete render_interface_;
    delete font_interface_;
}

void RmlUiLayer::OnUpdate(float ts) {
    if (context_)
        context_->Update();
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

void RmlUiLayer::OnEvent(Event& event) {
    EventDispatcher dispatcher(event);
    
    dispatcher.Dispatch<WindowResizeEvent>(SE_BIND_EVENT_FN(RmlUiLayer::OnWindowResize));
    dispatcher.Dispatch<MouseMovedEvent>(SE_BIND_EVENT_FN(RmlUiLayer::OnMouseMove));
    dispatcher.Dispatch<MouseButtonPressedEvent>(SE_BIND_EVENT_FN(RmlUiLayer::OnMouseButtonPressed));
    dispatcher.Dispatch<MouseButtonReleasedEvent>(SE_BIND_EVENT_FN(RmlUiLayer::OnMouseButtonReleased));
    dispatcher.Dispatch<MouseScrolledEvent>(SE_BIND_EVENT_FN(RmlUiLayer::OnMouseScrolled));
    dispatcher.Dispatch<KeyPressedEvent>(SE_BIND_EVENT_FN(RmlUiLayer::OnKeyPressed));
    dispatcher.Dispatch<KeyReleasedEvent>(SE_BIND_EVENT_FN(RmlUiLayer::OnKeyReleased));
    dispatcher.Dispatch<KeyTypedEvent>(SE_BIND_EVENT_FN(RmlUiLayer::OnKeyTyped));
}

bool RmlUiLayer::OnWindowResize(WindowResizeEvent& e) {
    if (context_) {
        context_->SetDimensions(Rml::Vector2i(e.GetWidth(), e.GetHeight()));
        render_interface_->SetViewport(e.GetWidth(), e.GetHeight());
    }
    return false;
}

bool RmlUiLayer::OnMouseMove(MouseMovedEvent& e) {
    if (context_)
        context_->ProcessMouseMove(e.GetX(), e.GetY(), 0); // Modifiers?
    return false; // Should we block? Maybe if hovering UI.
}

bool RmlUiLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e) {
    if (context_)
        context_->ProcessMouseButtonDown(e.GetMouseButton(), 0);
    return false;
}

bool RmlUiLayer::OnMouseButtonReleased(MouseButtonReleasedEvent& e) {
    if (context_)
        context_->ProcessMouseButtonUp(e.GetMouseButton(), 0);
    return false;
}

bool RmlUiLayer::OnMouseScrolled(MouseScrolledEvent& e) {
    if (context_)
        context_->ProcessMouseWheel(-e.GetYOffset(), 0); // RmlUi expects negative for up? Check docs.
    return false;
}

bool RmlUiLayer::OnKeyPressed(KeyPressedEvent& e) {
    if (context_) {
        // Map GLFW keys to RmlUi keys
        // This is tedious, need a mapper.
        // For now, pass raw if possible or minimal mapping.
        // Rml::Input::KeyIdentifier key = ConvertKey(e.GetKeyCode());
        // context_->ProcessKeyDown(key, 0);
    }
    return false;
}

bool RmlUiLayer::OnKeyReleased(KeyReleasedEvent& e) {
    if (context_) {
        // context_->ProcessKeyUp(key, 0);
    }
    return false;
}

bool RmlUiLayer::OnKeyTyped(KeyTypedEvent& e) {
    if (context_) {
        if (e.GetKeyCode() >= 32) // Printable
            context_->ProcessTextInput((Rml::Character)e.GetKeyCode());
    }
    return false;
}

}  // namespace se
