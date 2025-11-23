#pragma once

#include <engine/Camera.h>
#include <engine/Layer.h>
#include <engine/ecs/Entity.h>
#include <engine/ecs/Scene.h>
#include <engine/renderer/Material.h>

#include <glm.hpp>

using namespace se;

class ThirdPersonLayer : public Layer {
   public:
    ThirdPersonLayer();
    virtual ~ThirdPersonLayer();

    virtual void OnAttach() override;
    virtual void OnDetach() override;
    virtual void OnUpdate(float ts) override;
    virtual void OnRender() override;
    virtual void OnImGuiRender() override;

   private:
    void CreateScene();
    void UpdatePlayer(float ts);
    void UpdateCamera();

    std::shared_ptr<Scene>    scene_;
    std::shared_ptr<Material> material_;
    Camera                    camera_;

    // Player
    Entity    player_entity_;
    Entity    sun_entity_;
    glm::vec3 playerVelocity_{0.0f};
    bool      isGrounded_ = false;

    // Physics constants
    const float gravity_     = 20.0f;
    const float jumpForce_   = 10.0f;
    const float moveSpeed_   = 5.0f;
    const float floorHeight_ = 0.0f;  // Simple floor at y=0

    // Camera settings
    float cameraDistance_ = 10.0f;
    float cameraHeight_   = 5.0f;
    float cameraAngle_    = 0.0f;
    bool  camera_active_  = true;
};
