#pragma once

#include <engine/Layer.h>
#include <engine/ecs/Scene.h>
#include <engine/ecs/Entity.h>
#include <engine/renderer/MaterialHandle.h>

#include <glm.hpp>
#include <vector>

class btRigidBody;

using namespace se;

/**
 * Third-person player controller layer, now using Filament rendering.
 *
 * Replaces legacy OpenGL rendering with:
 * - MeshSystem for procedural geometry (floor, walls, player capsule)
 * - MaterialSystem for PBR materials
 * - LightSystem for directional light
 * - FilamentRenderer for camera control
 *
 * Physics (Bullet), input, and gameplay logic remain unchanged.
 */
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
    float debugTargetYaw_   = 0.0f;
    float debugCurrentYaw_  = 0.0f;
    float debugNewYaw_      = 0.0f;
    float debugPhysicsYaw_  = 0.0f;
    float debugPitch_       = 0.0f;
    float debugRoll_        = 0.0f;
    float lastRotationDiff_ = 0.0f;
    float lastShootTime_    = 0.0f;

    void CreateScene();
    void UpdatePlayer(float ts);
    void UpdateCamera();
    void Shoot();
    void UpdateGrabSystem(float ts);
    void TryGrabOrRelease();

    std::shared_ptr<Scene> scene_;

    // Materials
    MaterialHandle floorMaterial_;
    MaterialHandle playerMaterial_;
    MaterialHandle wallMaterial_;
    MaterialHandle cubeMaterial_;
    MaterialHandle bulletMaterial_;

    // Player entity (for physics + transform)
    Entity              playerEntity_;
    Entity              cube_entity_;
    Entity              floor_entity_;
    std::vector<Entity> walls_;
    std::vector<Entity> smallWalls_;
    std::vector<Entity> bullets_;
    glm::vec3           playerVelocity_{0.0f};
    bool                isGrounded_ = false;

    // Physics constants
    const float gravity_     = 20.0f;
    const float jumpForce_   = 10.0f;
    const float moveSpeed_   = 5.0f;
    const float sprintSpeed_ = 12.0f;
    const float floorHeight_ = 0.0f;

    // Camera settings (spring arm style)
    float springArmYaw_     = 0.0f;
    float springArmPitch_   = -30.0f;
    float springArmLength_  = 8.0f;
    float currentArmLength_ = 8.0f;
    glm::vec3 socketOffset_ = {0.0f, 1.5f, 0.0f};

    // Grab system (Half-Life style)
    btRigidBody* grabbedBody_     = nullptr;
    float        grabDistance_    = 100.0f;
    float        grabMaxDistance_ = 30.0f;
    float        grabMinDistance_ = 2.0f;
    glm::vec3    savedGravity_{0.0f};

    // Camera state for shooting/grab raycasts
    glm::vec3 cameraPosition_{0.0f, 5.0f, 10.0f};
    float     cameraYaw_   = 0.0f;
    float     cameraPitch_ = 0.0f;

    // Mouse toggle
    bool mouseCaptured_ = true;

    int maxBullets_ = 32;

    // Filament runtime tuning panel (ImGui)
    bool showImGuiDemo_ = false;
};
