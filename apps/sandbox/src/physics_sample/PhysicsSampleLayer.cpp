#include "PhysicsSampleLayer.h"

#include "engine/Application.h"
#include "engine/ecs/Components.h"
#include "engine/ecs/Scene.h"
#include "engine/new_event_system/EventBus.h"

PhysicsSampleLayer::~PhysicsSampleLayer()
{
}

void PhysicsSampleLayer::OnDetach()
{
    Layer::OnDetach();
}

void PhysicsSampleLayer::OnAttach()
{
    Layer::OnAttach();

    scene_ = CreateScope<Scene>();
    player_entity_ = scene_->CreateEntity("Player Entity");

    RigidbodyData data{
        .mass = 1.0f,
        .gravityScale = 1.0f
    };

    auto rig = player_entity_.AddComponent<RigidBodyComponent>(data);
    rig.PrintName();
}

void PhysicsSampleLayer::OnUpdate(float ts)
{
    Layer::OnUpdate(ts);
}

void PhysicsSampleLayer::OnRender()
{
    Layer::OnRender();
}

void PhysicsSampleLayer::OnImGuiRender()
{
    Layer::OnImGuiRender();
}

void PhysicsSampleLayer::OnEvent(Event& event)
{
    Layer::OnEvent(event);
}
