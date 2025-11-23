#include "engine/ui/RmlUiLayer.h"

#include <GLFW/glfw3.h>
#include <RmlUi/Debugger.h>

#include "engine/Application.h"
#include "engine/Log.h"
#include "engine/new_event_system/InputEvents.h"
#include "engine/new_event_system/NewApplicationEvents.h"

namespace se {

RmlUiLayer::RmlUiLayer() : Layer("RmlUiLayer") {}

RmlUiLayer::~RmlUiLayer() {}

void RmlUiLayer::OnAttach() {
    system_interface_ = new RmlUiSystemInterface();
    render_interface_ = new RmlUiRenderInterface();

    Rml::SetSystemInterface(system_interface_);
    Rml::SetRenderInterface(render_interface_);

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

    const std::string font_path = "assets/fonts/Rubik-Regular.ttf";
    if (!Rml::LoadFontFace(font_path)) {
        SE_LOG_ERROR("[RmlUiLayer] Failed to load font '{}'", font_path);
    } else {
        // Register the family name so we can use it in RML/RCSS
        // The family name is taken from the font’s internal name; you can also
        // force a name with the overload that takes a family string.
        // Here we simply rely on the default family that the TTF defines.
        SE_LOG_INFO("[RmlUiLayer] Font '{}' loaded successfully", font_path);
    }

    Rml::Debugger::Initialise(context_);

    // Subscribe to events via EventBus
    auto& event_bus = Application::Get().GetEventBus();

    resize_listener_id_ = event_bus.AddListener<NewWindowResizeEvent>([this](const NewWindowResizeEvent& e) {
        SE_LOG_INFO("[RmlUiLayer] Window Resize: {}x{}", e.width, e.height);
        if (context_) {
            context_->SetDimensions(Rml::Vector2i(e.width, e.height));
            render_interface_->SetViewport(e.width, e.height);
        }
    });

    mouse_moved_listener_id_ = event_bus.AddListener<NewMouseMovedEvent>([this](const NewMouseMovedEvent& e) {
        SE_LOG_INFO("[RmlUiLayer] Mouse Moved: ({}, {})", e.x, e.y);
        if (context_) context_->ProcessMouseMove(static_cast<int>(e.x), static_cast<int>(e.y), 0);
    });

    mouse_pressed_listener_id_ = event_bus.AddListener<NewMouseButtonPressedEvent>([this](const NewMouseButtonPressedEvent& e) {
        SE_LOG_INFO("[RmlUiLayer] Mouse Button Pressed: {}", static_cast<int>(e.button));
        if (context_) context_->ProcessMouseButtonDown(static_cast<int>(e.button), 0);
    });

    mouse_released_listener_id_ = event_bus.AddListener<NewMouseButtonReleasedEvent>([this](const NewMouseButtonReleasedEvent& e) {
        SE_LOG_INFO("[RmlUiLayer] Mouse Button Released: {}", static_cast<int>(e.button));
        if (context_) context_->ProcessMouseButtonUp(static_cast<int>(e.button), 0);
    });

    mouse_scrolled_listener_id_ = event_bus.AddListener<NewMouseScrolledEvent>([this](const NewMouseScrolledEvent& e) {
        SE_LOG_INFO("[RmlUiLayer] Mouse Scrolled: ({}, {})", e.xOffset, e.yOffset);
        if (context_) context_->ProcessMouseWheel(Rml::Vector2f(e.xOffset, -e.yOffset), 0);
    });

    key_pressed_listener_id_ = event_bus.AddListener<NewKeyPressedEvent>([this](const NewKeyPressedEvent& e) {
        SE_LOG_INFO("[RmlUiLayer] Key Pressed: {} (scancode: {}, mods: {})", static_cast<int>(e.key), e.scancode, e.mods);
        // TODO: Map GLFW keys to RmlUi keys
        // Rml::Input::KeyIdentifier key = ConvertKey(e.key);
        // context_->ProcessKeyDown(key, e.mods);
    });

    key_released_listener_id_ = event_bus.AddListener<NewKeyReleasedEvent>([this](const NewKeyReleasedEvent& e) {
        SE_LOG_INFO("[RmlUiLayer] Key Released: {} (scancode: {}, mods: {})", static_cast<int>(e.key), e.scancode, e.mods);
        // TODO: Map GLFW keys to RmlUi keys
        // context_->ProcessKeyUp(key, e.mods);
    });

    char_typed_listener_id_ = event_bus.AddListener<NewCharTypedEvent>([this](const NewCharTypedEvent& e) {
        SE_LOG_INFO("[RmlUiLayer] Char Typed: {} (0x{:X})", static_cast<char>(e.character), e.character);
        if (context_) {
            if (e.character >= 32)  // Printable
                context_->ProcessTextInput(static_cast<Rml::Character>(e.character));
        }
    });

    SE_LOG_INFO("[RmlUiLayer] Successfully attached and subscribed to input events");
}

void RmlUiLayer::OnDetach() {
    // Unsubscribe from all events
    auto& event_bus = Application::Get().GetEventBus();
    event_bus.RemoveListener<NewWindowResizeEvent>(resize_listener_id_);
    event_bus.RemoveListener<NewMouseMovedEvent>(mouse_moved_listener_id_);
    event_bus.RemoveListener<NewMouseButtonPressedEvent>(mouse_pressed_listener_id_);
    event_bus.RemoveListener<NewMouseButtonReleasedEvent>(mouse_released_listener_id_);
    event_bus.RemoveListener<NewMouseScrolledEvent>(mouse_scrolled_listener_id_);
    event_bus.RemoveListener<NewKeyPressedEvent>(key_pressed_listener_id_);
    event_bus.RemoveListener<NewKeyReleasedEvent>(key_released_listener_id_);
    event_bus.RemoveListener<NewCharTypedEvent>(char_typed_listener_id_);

    Rml::Shutdown();
    delete system_interface_;
    delete render_interface_;
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

}  // namespace se
