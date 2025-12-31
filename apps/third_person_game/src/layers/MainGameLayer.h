#pragma once
/**
 * MainGameLayer - Primary game layer managing scene lifecycle.
 */

#include "../components/Character.h"
#include "engine/Camera.h"
#include "engine/Layer.h"
#include "engine/ecs/Scene.h"
#include "engine/renderer/Material.h"

namespace FirstGame {
class MainGameLayer : public se::Layer {
public:
    ~MainGameLayer() override;

    void OnAttach() override;

    void OnDetach() override;

    void OnUpdate(float ts) override;

    void OnRender() override;

    void OnImGuiRender() override;

private:
    void ImguiDebug();

    se::Entity        character_entity_;
    se::Scope<se::Scene>  scene_;
    se::Ref<se::Material> material_;
    se::Entity    lightEntity_;
};
} // namespace FirstGame