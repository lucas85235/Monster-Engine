#pragma once
/**
 * MainGameLayer.h - Primary game layer for the third-person game.
 *
 * Manages scene lifecycle, entity creation, and game loop integration.
 * Loads map data and spawns the player character with all required components.
 */

#include "../components/Character.h"
#include "engine/renderer/Camera.h"
#include "engine/core/Layer.h"
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
    Entity        character_entity_;
    Scope<Scene>  scene_;
    Ref<Material> material_;
};
} // namespace FirstGame