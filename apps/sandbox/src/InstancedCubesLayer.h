#pragma once

#include <engine/core/Layer.h>
#include <engine/renderer/Camera.h>
#include <engine/renderer/InstancedMesh.h>
#include <engine/renderer/Material.h>
#include <engine/renderer/VertexArray.h>

class InstancedCubesLayer : public se::Layer {
public:
    InstancedCubesLayer();
    ~InstancedCubesLayer() override = default;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float ts) override;
    void OnRender() override;
    void OnImGuiRender() override;

private:
    Camera camera_;
    std::shared_ptr<se::VertexArray> cubeVAO_;
    std::shared_ptr<se::Material> material_;
    std::shared_ptr<se::InstancedMesh> instancedMesh_;
    float rotation_ = 0.0f;
    uint32_t instanceCount_ = 10;
    
    // Draw call tracking for verification
    uint32_t lastDrawCallCount_ = 0;
    uint32_t drawCallCountBeforeRender_ = 0;
};
