#include "InputSampleLayer.h"

#include "engine/event/KeyEvent.h"
#include "engine/event/MouseEvent.h"
#include "engine/input/Input.h"
#include "engine/input/KeyCodes.h"

InputSampleLayer::InputSampleLayer() : Layer("InputSampleLayer") {}
InputSampleLayer::~InputSampleLayer() {}
void InputSampleLayer::OnAttach() {
    Layer::OnAttach();

    // initialize common material and scene
    Ref<Material> material = Utilities::LoadMaterial();
    scene_                 = CreateScope<Scene>("Main Scene");

    // add the entities
    Utilities::CreateCubeEntity("Cube", {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, scene_.get(), material);
}
void InputSampleLayer::OnDetach() {
    Layer::OnDetach();
}
void InputSampleLayer::OnUpdate(float ts) {
    Layer::OnUpdate(ts);

    UpdateCameraInput();
}
void InputSampleLayer::OnRender() {
    Layer::OnRender();
}
void InputSampleLayer::OnImGuiRender() {
    Layer::OnImGuiRender();
}
void InputSampleLayer::OnEvent(Event& e) {
    // scene_->OnEvent(e);
    EventDispatcher dispatcher(e);
    dispatcher.Dispatch<KeyPressedEvent>(SE_BIND_EVENT_FN(InputSampleLayer::OnKeyPressedEvent));
    dispatcher.Dispatch<MouseButtonPressedEvent>(SE_BIND_EVENT_FN(InputSampleLayer::OnMouseButtonPressed));
}

bool InputSampleLayer::OnKeyPressedEvent(KeyPressedEvent& e) {
    // with a modifier logic and no repeat
    if (e.GetRepeatCount() == 0 && Input::IsKeyDown(Key::LeftControl)) {
        switch (e.GetKeyCode()) {
            case Key::F3:
                SE_LOG_INFO("F3");
                break;
        }
    }

    // with repeat logic
    switch (e.GetKeyCode()) {
        case Key::K:
            SE_LOG_INFO("K");
            break;
    }

    return false;
}

bool InputSampleLayer::OnMouseButtonPressed(MouseButtonPressedEvent& e) {
    return false;
}
void InputSampleLayer::UpdateCameraInput() {
    // every single frame
    if (Input::IsKeyDown(Key::W)) {
        SE_LOG_INFO("W");
    }
    if (Input::IsKeyDown(Key::S)) {
        SE_LOG_INFO("S");
    }
    if (Input::IsKeyDown(Key::D)) {
        SE_LOG_INFO("D");
    }
    if (Input::IsKeyDown(Key::A)) {
        SE_LOG_INFO("A");
    }
}