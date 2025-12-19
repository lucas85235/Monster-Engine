#pragma once

#include <engine/core/Layer.h>
#include <engine/renderer/Camera.h>
#include <engine/ecs/Scene.h>
#include <engine/renderer/Material.h>
#include <engine/ecs/Entity.h>
#include <glm.hpp>
#include <vector>

class btRigidBody;

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
    float debugTargetYaw_ = 0.0f;
    float debugCurrentYaw_ = 0.0f;
    float debugNewYaw_ = 0.0f;
    float debugPhysicsYaw_ = 0.0f;
    float debugPitch_ = 0.0f;
    float debugRoll_ = 0.0f;
    float lastRotationDiff_ = 0.0f;
    float lastShootTime_ = 0.0f;

    void CreateScene();
    void UpdatePlayer(float ts);
    void UpdateCamera();
    void Shoot();
    void UpdateGrabSystem(float ts);
    void TryGrabOrRelease();
    void CleanupBullets();

    std::shared_ptr<Scene> scene_;
    std::shared_ptr<Material> material_;
    Camera camera_;

    // Player
    Entity playerEntity_;
    Entity cube_entity_;
    Entity floor_entity_;
    std::vector<Entity> walls_;
    std::vector<Entity> bullets_;  // Track shot cubes for cleanup
    glm::vec3 playerVelocity_{0.0f};
    bool isGrounded_ = false;
    
    // Physics constants
    const float gravity_ = 20.0f;
    const float jumpForce_ = 10.0f;
    const float moveSpeed_ = 5.0f;
    const float sprintSpeed_ = 12.0f;  // Shift speed
    const float floorHeight_ = 0.0f;

    // Camera settings
    float cameraDistance_ = 10.0f;
    float cameraHeight_ = 5.0f;
    float cameraAngle_ = 0.0f;

    // Grab system (Half-Life style)
    btRigidBody* grabbedBody_ = nullptr;
    float grabDistance_ = 100.0f;
    float grabMaxDistance_ = 30.0f;
    float grabMinDistance_ = 2.0f;    // Minimum grab distance (scroll wheel)
    glm::vec3 savedGravity_{0.0f};
    
    // Mouse toggle
    bool mouseCaptured_ = true;

    // Culling settings (exposed via ImGui)
    bool enableFrustumCulling_ = true;
    bool enableOcclusionCulling_ = true;
    int maxBullets_ = 100;
};


