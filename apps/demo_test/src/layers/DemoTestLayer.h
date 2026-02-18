#pragma once
#include "engine/Camera.h"
#include "engine/Layer.h"
#include "engine/ecs/Scene.h"
#include "engine/renderer/Material.h"
#include "engine/gameplay/Gameplay.h"

namespace DemoTest {

class DemoTestLayer : public se::Layer {
public:
    DemoTestLayer() = default;
    ~DemoTestLayer() override;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float ts) override;
    void OnRender() override;
    void OnImGuiRender() override;

private:

};

} // namespace AnimationTest
