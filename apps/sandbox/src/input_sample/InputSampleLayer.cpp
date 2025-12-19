#include "InputSampleLayer.h"

#include "engine/core/Application.h"
#include "engine/events/Events.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCodes.h"

InputSampleLayer::InputSampleLayer() : Layer("InputSampleLayer") {}
InputSampleLayer::~InputSampleLayer() {}

void InputSampleLayer::OnAttach() {
    // Get EventBus from Application
    event_bus_ = &Application::Get().GetEventBus();

    // Subscribe to events via EventBus
    event_bus_->AddListener<KeyPressedEvent>(SE_BIND_EVENT_FN(OnKeyPressed));
    event_bus_->AddListener<MouseButtonPressedEvent>(SE_BIND_EVENT_FN(OnMouseButtonPressed));

    // Initialize common material and scene
    Ref<Material> material = Utilities::LoadMaterial();
    scene_                 = CreateScope<Scene>("Main Scene");

    // Add the entities
    Utilities::CreateCubeEntity("Cube", {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, scene_.get(), material);
}

void InputSampleLayer::OnDetach() {}

void InputSampleLayer::OnUpdate(float ts) {
    UpdateCameraInput();
}

void InputSampleLayer::OnRender() {}

void InputSampleLayer::OnImGuiRender() {}

void InputSampleLayer::OnKeyPressed(const KeyPressedEvent& e) {
    // With a modifier logic and no repeat
    if (!e.IsRepeat() && Input::IsKeyDown(Key::LeftControl)) {
        switch (e.keyCode) {
            case Key::F3:
                SE_LOG_INFO("F3");
                break;
        }
    }

    // With repeat logic
    switch (e.keyCode) {
        case Key::K:
            SE_LOG_INFO("K");
            break;
    }
}

void InputSampleLayer::OnMouseButtonPressed(const MouseButtonPressedEvent& e) {
    SE_LOG_INFO("Mouse button {} pressed", e.button);
}

void InputSampleLayer::UpdateCameraInput() {
    // Every single frame
    // if (Input::IsKeyDown(Key::W)) { SE_LOG_INFO("W"); }
}