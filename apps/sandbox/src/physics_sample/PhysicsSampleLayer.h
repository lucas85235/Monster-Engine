#pragma once
#include "engine/Layer.h"
#include "engine/new_event_system/EventBus.h"
#include "engine/new_event_system/NewApplicationEvents.h"
#include "engine/Camera.h"
#include "engine/renderer/Material.h"
using namespace se;

class PhysicsSampleLayer : public Layer
{
public:
    ~PhysicsSampleLayer() override;
    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float ts) override;
    void OnRender() override;
    void OnImGuiRender() override;
    void OnEvent(Event& event) override;

private:
    Entity player_entity_;
    Scope<Scene> scene_;
    Camera camera_;
    std::shared_ptr<Material> material_;
};
