#pragma once

#include "engine/Layer.h"
#include "engine/ecs/Scene.h"
#include "engine/ecs/Entity.h"

namespace se {
class EditorLayer;
}

namespace DemoTest {

/**
 * Demonstration layer showcasing the improved Monster Engine DX.
 *
 * Pure game logic — no ImGui code. The engine EditorLayer handles all
 * inspection/debug UI via docking panels.
 */
class DemoTestLayer : public se::Layer {
   public:
    DemoTestLayer() : Layer("DemoTestLayer") {}
    ~DemoTestLayer() override;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float ts) override;
    void OnRender() override;

    /** Return the scene so EditorLayer can inspect it. */
    se::Scene* GetScene() const { return scene_.get(); }

   private:
    std::shared_ptr<se::Scene> scene_;
    float animationTime_ = 0.0f;

    const std::string inputMapName_ = "demo_test";
};

}  // namespace DemoTest
