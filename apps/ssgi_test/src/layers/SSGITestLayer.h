#pragma once

#include "engine/Layer.h"
#include "engine/ecs/Scene.h"
#include "engine/renderer/Material.h"

#include <memory>
#include <glm.hpp>

class Camera;

namespace SSGITest {

class SSGITestLayer : public se::Layer {
public:
    SSGITestLayer() : Layer("SSGITestLayer") {}
    ~SSGITestLayer() override;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float ts) override;
    void OnRender() override;
    void OnImGuiRender() override;

private:
    void CreateMaterials();
    void SetupScene();
    void RenderDebugPanel();

    se::Scope<se::Scene> scene_;
    std::unique_ptr<Camera> camera_;
    se::Ref<se::Material> defaultMaterial_;
    
    // Camera controls (EditorCamera-style)
    float cameraYaw_ = -90.0f;
    float cameraPitch_ = -15.0f;
};

} // namespace SSGITest
