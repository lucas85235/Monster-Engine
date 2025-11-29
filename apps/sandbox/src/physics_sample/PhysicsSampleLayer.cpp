#include "PhysicsSampleLayer.h"

#include "engine/Application.h"
#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/new_event_system/EventBus.h"
#include "engine/physics/RigidbodyComponent.h"

PhysicsSampleLayer::~PhysicsSampleLayer() {}

void PhysicsSampleLayer::OnDetach() {
    Layer::OnDetach();
}

void PhysicsSampleLayer::OnAttach() {
    Layer::OnAttach();

    scene_         = CreateScope<Scene>();
    player_entity_ = scene_->CreateEntity("Player Entity");

    Application::Get().GetPhysicsManager().SetScene(*scene_);

    RigidbodyData data = RigidbodyData{.mass = 1.0f, .gravityScale = 1.0f};

    auto rig = player_entity_.AddComponent<RigidbodyComponent>(data, player_entity_);
}

void PhysicsSampleLayer::OnUpdate(float ts) {
    Layer::OnUpdate(ts);
}

void PhysicsSampleLayer::OnRender() {
    Layer::OnRender();
}

void PhysicsSampleLayer::OnImGuiRender() {
    Layer::OnImGuiRender();
}

void PhysicsSampleLayer::OnEvent(Event& event) {
    Layer::OnEvent(event);
}
