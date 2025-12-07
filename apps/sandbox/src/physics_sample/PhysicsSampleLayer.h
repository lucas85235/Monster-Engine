#pragma once
#include "engine/Layer.h"
#include "engine/events/EventBus.h"
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

private:
    Entity player_entity_;
    Scope<Scene> scene_;
    Camera camera_;
    std::shared_ptr<Material> material_;
};
