#pragma once
/**
 * AnimationTestLayer - Test layer for advanced animation system development.
 */

#include "engine/Camera.h"
#include "engine/Layer.h"
#include "engine/ecs/Scene.h"
#include "engine/renderer/Material.h"
#include "engine/gameplay/Gameplay.h"

namespace AnimationTest {

class AnimationTestLayer : public se::Layer {
public:
    AnimationTestLayer() = default;
    ~AnimationTestLayer() override;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float ts) override;
    void OnRender() override;
    void OnImGuiRender() override;

private:
    void SetupScene();
    void SetupPlayer();
    void SetupLighting();
    void RenderDebugUI();
    void RenderAnimationDebugPanel();
    void RenderBlendSpaceDebugPanel();
    void RenderLayerDebugPanel();

    se::Scope<se::Scene> scene_;
    se::Entity playerEntity_;
    se::Entity lightEntity_;
    
    bool showSkeletonDebug_ = false;
    bool showBlendSpaceDebug_ = true;
    bool showLayerDebug_ = true;
    bool showLookAtDebug_ = true;
};

} // namespace AnimationTest
