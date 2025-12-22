#pragma once

#include <engine/core/Layer.h>
#include <engine/renderer/Camera.h>
#include <engine/renderer/Material.h>
#include <engine/renderer/VertexArray.h>

class VulkanTestLayer : public se::Layer {
public:
    VulkanTestLayer();
    ~VulkanTestLayer() override = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float ts) override;
    void OnRender() override;
    void OnImGuiRender() override {}

private:
    Camera camera_;  // Camera is in global namespace, not se::
    std::shared_ptr<se::VertexArray> cubeVAO_;
    std::shared_ptr<se::Material> material_;
    float rotation_ = 0.0f;
};

