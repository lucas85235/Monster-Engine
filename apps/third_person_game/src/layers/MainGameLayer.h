#pragma once
/**
 * MainGameLayer - Primary game layer managing scene lifecycle.
 */

#include "engine/Camera.h"
#include "engine/Layer.h"
#include "engine/ecs/Scene.h"
#include "engine/renderer/Material.h"

// Gameplay System
#include "engine/gameplay/Gameplay.h"
#include "engine/ai/AI.h"
#include "engine/navigation/Navigation.h"

// Native UI System
#include "engine/ui/native/UISystem.h"
#include "engine/ui/native/hud/HUDController.h"

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
    void SetupHUD();
    void SetupPlayer();
    void SetupEnemy();
    void SetupNavigation();

    se::Scope<se::Scene>  scene_;
    se::Entity            playerEntity_;
    se::Entity            enemyEntity_;
    se::Entity            lightEntity_;
    se::Ref<se::Material> material_;
    
    // Navigation
    std::unique_ptr<se::nav::NavigationSystem> navSystem_;
    
    // HUD System
    std::unique_ptr<se::ui::HUDController> hudController_;
    
    // Demo state
    float demoHealth_ = 100.0f;
    bool animateHealth_ = false;
};
} // namespace FirstGame