#pragma once

#include "engine/Layer.h"
#include "engine/Camera.h"
#include "engine/ecs/Scene.h"
#include "engine/ecs/Entity.h"
#include "engine/renderer/Material.h"
#include "engine/renderer/PBRMaterial.h"

#include <memory>
#include <glm.hpp>

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
    
    // PBR material parameters for override testing
    se::PBRMaterialParams testMaterialParams_;
    
    // Camera controls (EditorCamera-style)
    float cameraYaw_ = -90.0f;
    float cameraPitch_ = -15.0f;
    float camera_speed_ = 5.0f;
    
    se::Entity lightEntity_;
};

} // namespace SSGITest
