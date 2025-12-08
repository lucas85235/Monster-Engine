#include "ThirdPersonLayer.h"

#include <engine/Application.h>
#include <engine/Log.h>
#include <engine/ecs/SimpleComponents.h>
#include <engine/input/InputManager.h>
#include <engine/resources/MeshManager.h>
#include <imgui.h>
#include <btBulletDynamicsCommon.h>

#include <gtc/matrix_transform.hpp>

#include "MathUtils.h"
#include "SampleUtilities.h"
#include "engine/physics/PhysicsSystem.h"
#include "engine/physics/PhysicsDebugDraw.h"
#include "engine/physics/RigidbodyComponent.h"
#include "engine/physics/BoxCollider.h"


ThirdPersonLayer::ThirdPersonLayer() : Layer("ThirdPersonLayer"), camera_(glm::vec3(0.0f, 5.0f, 10.0f)) {}

ThirdPersonLayer::~ThirdPersonLayer() {}

void ThirdPersonLayer::OnAttach() {
    SE_LOG_INFO("ThirdPersonLayer attached");

    material_ = Utilities::LoadMaterial();
    CreateScene();

    // Configure scene renderer culling
    auto& sceneRenderer = Application::Get().GetRenderer().GetSceneRenderer();
    sceneRenderer.SetFrustumCullingEnabled(enableFrustumCulling_);
    sceneRenderer.SetOcclusionCullingEnabled(enableOcclusionCulling_);

    // Bind input axes and actions
    auto& input = InputManager::Get();
    input.BindAxis("MoveForward", Key::W, 1.0f);
    input.BindAxis("MoveForward", Key::S, -1.0f);
    input.BindAxis("MoveRight", Key::D, 1.0f);
    input.BindAxis("MoveRight", Key::A, -1.0f);
    input.BindAction("Jump", Key::Space);
    input.BindAction("Grab", Key::E);
    input.BindAction("Sprint", Key::LeftShift);
    input.BindAction("ToggleMouse", Key::Tab);
    input.BindAxis("CameraRotateX", Key::MouseX, 1.0f);
    input.BindAxis("CameraRotateY", Key::MouseY, -1.0f);
    input.BindAxis("ScrollWheel", Key::MouseScrollY, 1.0f);

    // Capture and hide cursor
    auto& app    = Application::Get();
    auto* window = app.GetWindow().GetNativeWindow();
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    mouseCaptured_ = true;

    // physics debug drawing
    scene_->GetPhysicsSystem()->GetDebugDrawer()->setDebugMode(btIDebugDraw::DBG_NoDebug);
}

void ThirdPersonLayer::OnDetach() {
    bullets_.clear();
    scene_.reset();
}

void ThirdPersonLayer::CreateScene() {
    scene_ = CreateScope<Scene>("Third Person Scene");

    // Create floor and walls
    {
        floor_entity_ = scene_->CreateEntity("Floor");
        auto mesh     = MeshManager::GetPrimitive(PrimitiveMeshType::Cube);
        floor_entity_.AddComponent<MeshRenderComponent>(mesh, material_);

        auto& transform = floor_entity_.GetComponent<TransformComponent>();
        transform.SetPosition({0.0f, -1.0f, 0.0f});
        transform.SetScale({50.0f, 2.0f, 50.0f});

        RigidbodyData data = RigidbodyData{.mass = 0.0f, .gravityScale = 1.0f};
        floor_entity_.AddComponent<RigidbodyComponent>(data, floor_entity_);

        // Create walls - data-driven approach
        struct WallDef {
            const char* name;
            glm::vec3 position;
            glm::vec3 scale;
        };

        const WallDef wallDefinitions[] = {
            {"Wall_North", {0.0f, 25.0f, 25.0f}, {50.0f, 50.0f, 1.0f}},
            {"Wall_South", {0.0f, 25.0f, -25.0f}, {50.0f, 50.0f, 1.0f}},
            {"Wall_East", {25.0f, 25.0f, 0.0f}, {1.0f, 50.0f, 50.0f}},
            {"Wall_West", {-25.0f, 25.0f, 0.0f}, {1.0f, 50.0f, 50.0f}},
        };

        walls_.reserve(std::size(wallDefinitions));

        for (const auto& def : wallDefinitions) {
            auto wall = scene_->CreateEntity(def.name);
            wall.AddComponent<MeshRenderComponent>(mesh, material_);

            auto& wallTransform = wall.GetComponent<TransformComponent>();
            wallTransform.SetPosition(def.position);
            wallTransform.SetScale(def.scale);

            wall.AddComponent<RigidbodyComponent>(data, wall);
            walls_.emplace_back(wall);
        }
    }

    // Create player capsule
    {
        playerEntity_ = scene_->CreateEntity("Player");
        auto mesh     = MeshManager::GetPrimitive(PrimitiveMeshType::Capsule);

        if (!material_) {
            SE_LOG_ERROR("Material not loaded, retrying");
            material_ = Utilities::LoadMaterial();
        }

        playerEntity_.AddComponent<MeshRenderComponent>(mesh, material_);

        // Add Capsule Collider
        playerEntity_.AddComponent<CapsuleCollider>(0.5f, 1.0f);

        auto& transform = playerEntity_.GetComponent<TransformComponent>();
        transform.SetPosition({0.0f, 5.0f, 0.0f});
        transform.SetScale({1.0f, 1.0f, 1.0f});

        RigidbodyData data = RigidbodyData{.mass = 10.0f, .gravityScale = 1.0f};
        auto& rb = playerEntity_.AddComponent<RigidbodyComponent>(data, playerEntity_);
        
        // Lock rotation to prevent tipping over
        rb.SetAngularFactor({0.0f, 1.0f, 0.0f});

        // Add camera spring arm
        auto& springArm           = playerEntity_.AddComponent<SpringArmComponent>();
        springArm.TargetArmLength = 8.0f;
        springArm.SocketOffset    = {0.0f, 1.5f, 0.0f};
        springArm.Pitch           = -30.0f;
    }

    // Create directional light
    {
        auto  light     = scene_->CreateEntity("Sun");
        auto& transform = light.GetComponent<TransformComponent>();
        transform.SetPosition({10.0f, 20.0f, 10.0f});
        transform.SetRotation({45.0f, 45.0f, 0.0f});

        auto& dirLight       = light.AddComponent<DirectionalLightComponent>();
        dirLight.Color       = {1.0f, 0.9f, 0.8f};
        dirLight.Intensity   = 1.2f;
        dirLight.CastShadows = true;
    }

    {
        cube_entity_ = scene_->CreateEntity("Cube");
        auto mesh    = MeshManager::GetPrimitive(PrimitiveMeshType::Cube);

        if (!material_) {
            SE_LOG_ERROR("Material not loaded, retrying");
            material_ = Utilities::LoadMaterial();
        }

        cube_entity_.GetComponent<TransformComponent>().SetPosition({0.0f, 10.0f, 0.0f});

        cube_entity_.AddComponent<MeshRenderComponent>(mesh, material_);
        cube_entity_.AddComponent<BoxCollider>(Vector3(1.0f, 1.0f, 1.0f));

        RigidbodyData data;
        cube_entity_.AddComponent<RigidbodyComponent>(data, cube_entity_);
    }

    // Create small walls (muretas) for occlusion culling testing
    {
        struct WallConfig {
            glm::vec3 position;
            glm::vec3 scale;
            float yRotation;
        };

        std::vector<WallConfig> smallWalls = {
            {{10.0f, 1.5f, 0.0f}, {5.0f, 3.0f, 0.5f}, 0.0f},      // Right of spawn
            {{-10.0f, 1.5f, 5.0f}, {5.0f, 3.0f, 0.5f}, 45.0f},    // Left angled
            {{0.0f, 1.5f, -12.0f}, {8.0f, 3.0f, 0.5f}, 0.0f},     // Behind spawn
            {{15.0f, 1.5f, 15.0f}, {6.0f, 3.0f, 0.5f}, 30.0f},    // Far corner
            {{-8.0f, 1.5f, -8.0f}, {4.0f, 3.0f, 0.5f}, -45.0f},   // Diagonal
        };

        for (size_t i = 0; i < smallWalls.size(); i++) {
            const auto& config = smallWalls[i];
            
            auto wall = scene_->CreateEntity("SmallWall_" + std::to_string(i));
            auto mesh = MeshManager::GetPrimitive(PrimitiveMeshType::Cube);
            wall.AddComponent<MeshRenderComponent>(mesh, material_);

            auto& transform = wall.GetComponent<TransformComponent>();
            transform.SetPosition(config.position);
            transform.SetScale(config.scale);
            transform.SetRotation({0.0f, config.yRotation, 0.0f});

            // BoxCollider uses half-extents relative to unit cube
            wall.AddComponent<BoxCollider>(glm::vec3(1.0f));

            RigidbodyData data;
            data.mass = 0.0f;  // Static
            wall.AddComponent<RigidbodyComponent>(data, wall);
        }
        
        SE_LOG_INFO("Created {} small walls for occlusion testing", smallWalls.size());
    }
}

void ThirdPersonLayer::CleanupBullets() {
    // Bullets are not cleaned up - they persist for physics testing
    // Just remove invalid entity references from tracking
    if (!scene_) return;
    
    bullets_.erase(
        std::remove_if(bullets_.begin(), bullets_.end(), 
            [](const Entity& e) { return !e.IsValid(); }),
        bullets_.end()
    );
}

void ThirdPersonLayer::OnUpdate(float ts) {
    UpdatePlayer(ts);
    UpdateGrabSystem(ts);
    CleanupBullets();  // Only removes invalid references, not actual bullets
    scene_->OnUpdate(ts);
    UpdateCamera();
}

void ThirdPersonLayer::UpdatePlayer(float ts) {
    auto& input     = InputManager::Get();
    auto& transform = playerEntity_.GetComponent<TransformComponent>();
    auto& springArm = playerEntity_.GetComponent<SpringArmComponent>();
    auto& rb        = playerEntity_.GetComponent<RigidbodyComponent>();

    // Calculate camera-relative movement direction
    float moveForward = input.GetAxis("MoveForward");
    float moveRight   = input.GetAxis("MoveRight");

    float yawRad = springArm.Yaw * 0.0174533f;
    float sinYaw = std::sin(yawRad);
    float cosYaw = std::cos(yawRad);

    glm::vec3 camForward = {-sinYaw, 0.0f, -cosYaw};
    glm::vec3 camRight   = {cosYaw, 0.0f, -sinYaw};

    glm::vec3 movement = (camForward * moveForward + camRight * moveRight);
    
    btVector3 currentVelocity = rb.GetLinearVelocity();
    btVector3 desiredVelocity(0, currentVelocity.y(), 0);

    if (glm::length(movement) > 0.01f) {
        movement = glm::normalize(movement);

        // Smoothly rotate player to face movement direction
        float targetYaw     = Math::CalculateYawFromDirection(movement.x, movement.z);
        float currentYaw    = transform.Rotation.y;
        
        // Hysteresis for 180 degree turns
        float diff = targetYaw - currentYaw;
        diff = Math::NormalizeAngle(diff);
        
        // If we are near the singularity (180 degrees), favor the previous direction
        if (std::abs(diff) > 170.0f && std::abs(lastRotationDiff_) > 0.0f) {
            // If signs match, we are good. If signs differ, we might be flipping.
            // Force diff to have the same sign as lastRotationDiff_
            if ((diff > 0 && lastRotationDiff_ < 0) || (diff < 0 && lastRotationDiff_ > 0)) {
                if (diff > 0) diff -= 360.0f;
                else diff += 360.0f;
            }
        }
        lastRotationDiff_ = diff;

        float rotationSpeed = 10.0f;
        float newYaw = currentYaw + diff * glm::clamp(rotationSpeed * ts, 0.0f, 1.0f);
        newYaw = Math::NormalizeAngle(newYaw);

        // Apply rotation to Rigidbody
        rb.SetRotation({0.0f, newYaw, 0.0f});

        // Debug capture
        debugTargetYaw_ = targetYaw;
        debugCurrentYaw_ = currentYaw;
        debugNewYaw_ = newYaw;
    }

    // Ground Check using Raycast
    glm::vec3 rayStart = transform.Position + glm::vec3(0.0f, 1.0f, 0.0f); // Center of capsule
    glm::vec3 rayEnd   = transform.Position + glm::vec3(0.0f, -1.2f, 0.0f); // Below feet
    glm::vec3 hitPoint, hitNormal;

    bool hit = scene_->GetPhysicsSystem()->Raycast(rayStart, rayEnd, hitPoint, hitNormal, rb.GetRigidbody());
    isGrounded_ = hit;

    // Jumping
    if (input.IsActionJustPressed("Jump") && isGrounded_) {
        desiredVelocity.setY(5.0f); // Jump impulse/velocity
    }

    // Apply Velocity
    // We only control X and Z velocity directly. Y is controlled by gravity/jump unless we are grounded.
    // If we are grounded, we might want to stick to the ground or just let physics handle it.
    // For a simple character controller, setting linear velocity directly is often easiest but can fight with collisions.
    // Better approach: Set X/Z velocity, keep existing Y velocity (unless jumping).
    
    if (!isGrounded_) {
        // Keep existing Y if in air (gravity)
        desiredVelocity.setY(currentVelocity.y());
        
        // If we just jumped, we already set Y to 5.0f above.
        if (input.IsActionJustPressed("Jump") && isGrounded_) {
             desiredVelocity.setY(5.0f);
        }
    } else {
        // If grounded, we can still have some Y velocity from slopes, but mostly we want to stick.
        // If we are jumping, we override Y.
        if (input.IsActionJustPressed("Jump")) {
             desiredVelocity.setY(5.0f);
        }
    }

    // Apply movement to desired velocity - Sprint with Shift
    float speed = input.IsActionPressed("Sprint") ? sprintSpeed_ : moveSpeed_;
    desiredVelocity.setX(movement.x * speed);
    desiredVelocity.setZ(movement.z * speed);

    // Set the velocity on the rigidbody
    rb.SetLinearVelocity(desiredVelocity);

    // Shooting
    if (input.IsMouseButtonDown(0)) { // Left Mouse Button
        Shoot();
    }

    // Toggle mouse capture with Tab
    if (input.IsActionJustPressed("ToggleMouse")) {
        auto& app = Application::Get();
        auto* window = app.GetWindow().GetNativeWindow();
        mouseCaptured_ = !mouseCaptured_;
        glfwSetInputMode(window, GLFW_CURSOR, mouseCaptured_ ? GLFW_CURSOR_DISABLED : GLFW_CURSOR_NORMAL);
        SE_LOG_INFO("Mouse capture: {}", mouseCaptured_ ? "enabled" : "disabled");
    }

    // Grab/Release with E
    if (input.IsActionJustPressed("Grab")) {
        TryGrabOrRelease();
    }
}

void ThirdPersonLayer::Shoot() {
    float time = (float)glfwGetTime();
    if (time - lastShootTime_ < 0.02f) return; // 0.2s cooldown
    lastShootTime_ = time;

    // Get camera forward
    float yawRad = glm::radians(camera_.GetYaw());
    float pitchRad = glm::radians(camera_.GetPitch());
    glm::vec3 forward;
    forward.x = cos(yawRad) * cos(pitchRad);
    forward.y = sin(pitchRad);
    forward.z = sin(yawRad) * cos(pitchRad);
    forward = glm::normalize(forward);

    // Spawn position
    glm::vec3 spawnPos = camera_.GetPosition() + forward * 10.0f;

    // Create Entity
    auto box = scene_->CreateEntity("BulletBox");
    auto mesh = MeshManager::GetPrimitive(PrimitiveMeshType::Cube);
    box.AddComponent<MeshRenderComponent>(mesh, material_);
    box.AddComponent<BoxCollider>(glm::vec3(1.0f)); 
    
    box.GetComponent<TransformComponent>().SetPosition(spawnPos);
    box.GetComponent<TransformComponent>().SetScale(glm::vec3(0.5f));

    RigidbodyData data;
    data.mass = 2.0f;
    auto& rb = box.AddComponent<RigidbodyComponent>(data, box);
    
    // Apply impulse
    btVector3 impulse(forward.x, forward.y, forward.z);
    impulse *= 50.0f; // Force
    rb.GetRigidbody()->applyCentralImpulse(impulse);

    // Track bullet for cleanup
    bullets_.push_back(box);
}


void ThirdPersonLayer::UpdateCamera() {
    auto& input = InputManager::Get();

    if (!playerEntity_.HasComponent<SpringArmComponent>()) return;
    auto& springArm   = playerEntity_.GetComponent<SpringArmComponent>();
    auto& playerTrans = playerEntity_.GetComponent<TransformComponent>();

    float mouseX = input.GetAxis("CameraRotateX");
    float mouseY = input.GetAxis("CameraRotateY");

    springArm.Yaw -= mouseX * 0.1f;
    springArm.Pitch -= mouseY * 0.1f;
    springArm.Pitch = glm::clamp(springArm.Pitch, springArm.MinPitch, springArm.MaxPitch);

    float yawRad   = glm::radians(springArm.Yaw);
    float pitchRad = glm::radians(springArm.Pitch);

    float sinYaw   = std::sin(yawRad);
    float cosYaw   = std::cos(yawRad);
    float sinPitch = std::sin(pitchRad);
    float cosPitch = std::cos(pitchRad);

    glm::vec3 direction;
    direction.x = cosPitch * sinYaw;
    direction.y = sinPitch;
    direction.z = cosPitch * cosYaw;

    glm::vec3 targetPos = playerTrans.Position + springArm.SocketOffset;
    
    float desiredArmLength = springArm.TargetArmLength;

    if (springArm.DoCollisionTest && scene_->GetPhysicsSystem()) {
        btRigidBody* playerBody = nullptr;
        if (playerEntity_.HasComponent<RigidbodyComponent>()) {
            playerBody = playerEntity_.GetComponent<RigidbodyComponent>().GetRigidbody();
        }

        glm::vec3 rayStart = targetPos;
        glm::vec3 rayEnd   = targetPos + direction * (springArm.TargetArmLength + springArm.ProbeSize);
        glm::vec3 hitPoint, hitNormal;

        bool hit = scene_->GetPhysicsSystem()->Raycast(rayStart, rayEnd, hitPoint, hitNormal, playerBody);

        if (hit) {
            float hitDistance = glm::length(hitPoint - rayStart) - springArm.ProbeSize;
            desiredArmLength = glm::max(hitDistance, 0.5f);
        }
    }

    float lerpSpeed = (desiredArmLength < springArm.CurrentArmLength) ? 15.0f : 5.0f;
    springArm.CurrentArmLength = glm::mix(springArm.CurrentArmLength, desiredArmLength, glm::clamp(lerpSpeed * (1.0f / 60.0f), 0.0f, 1.0f));

    glm::vec3 camPos = targetPos + direction * springArm.CurrentArmLength;

    camera_.SetPosition(camPos);
    camera_.SetYaw(-springArm.Yaw - 90.0f);
    camera_.SetPitch(-springArm.Pitch);
}

void ThirdPersonLayer::TryGrabOrRelease() {
    if (grabbedBody_) {
        // Release the object
        grabbedBody_->setGravity(btVector3(savedGravity_.x, savedGravity_.y, savedGravity_.z));
        grabbedBody_->setLinearVelocity(btVector3(0, 0, 0));
        grabbedBody_->activate();
        SE_LOG_INFO("Released grabbed object");
        grabbedBody_ = nullptr;
        return;
    }

    // Try to grab an object with raycast
    auto& rb = playerEntity_.GetComponent<RigidbodyComponent>();
    
    // Get camera forward direction
    float yawRad = glm::radians(camera_.GetYaw());
    float pitchRad = glm::radians(camera_.GetPitch());
    glm::vec3 forward;
    forward.x = cos(yawRad) * cos(pitchRad);
    forward.y = sin(pitchRad);
    forward.z = sin(yawRad) * cos(pitchRad);
    forward = glm::normalize(forward);

    glm::vec3 rayStart = camera_.GetPosition();
    glm::vec3 rayEnd = rayStart + forward * grabMaxDistance_;
    glm::vec3 hitPoint;

    btRigidBody* hitBody = scene_->GetPhysicsSystem()->RaycastHitBody(rayStart, rayEnd, hitPoint, rb.GetRigidbody());

    // Debug visualization - draw raycast line and hit point for 10 seconds
    auto* debugDraw = scene_->GetPhysicsSystem()->GetDebugDrawer();
    if (debugDraw) {
        if (hitBody) {
            // Green line to hit point
            debugDraw->DrawDebugLine(rayStart, hitPoint, glm::vec3(0.0f, 1.0f, 0.0f), 10.0f);
            // Green sphere at hit point
            debugDraw->DrawDebugSphere(hitPoint, 0.2f, glm::vec3(0.0f, 1.0f, 0.0f), 10.0f);
        } else {
            // Red line to end of raycast (no hit)
            debugDraw->DrawDebugLine(rayStart, rayEnd, glm::vec3(1.0f, 0.0f, 0.0f), 10.0f);
        }
    }

    if (hitBody && hitBody->getMass() > 0.0f) {
        // Don't grab static objects or the player
        if (hitBody == rb.GetRigidbody()) return;
        
        grabbedBody_ = hitBody;
        
        // Save and disable gravity
        btVector3 grav = grabbedBody_->getGravity();
        savedGravity_ = glm::vec3(grav.x(), grav.y(), grav.z());
        grabbedBody_->setGravity(btVector3(0, 0, 0));
        grabbedBody_->setActivationState(DISABLE_DEACTIVATION);
        
        // Calculate initial grab distance
        grabDistance_ = glm::length(hitPoint - rayStart);
        grabDistance_ = glm::clamp(grabDistance_, 2.0f, grabMaxDistance_);
        
        SE_LOG_INFO("Grabbed object at distance {:.2f}", grabDistance_);
    }
}

void ThirdPersonLayer::UpdateGrabSystem(float ts) {
    if (!grabbedBody_) return;

    // Adjust grab distance with mouse scroll wheel
    auto& input = InputManager::Get();
    float scroll = input.GetAxis("ScrollWheel");
    if (std::abs(scroll) > 0.01f) {
        grabDistance_ -= scroll * 2.0f;  // Scroll up = closer
        grabDistance_ = glm::clamp(grabDistance_, grabMinDistance_, grabMaxDistance_);
    }

    // Get camera forward direction
    float yawRad = glm::radians(camera_.GetYaw());
    float pitchRad = glm::radians(camera_.GetPitch());
    glm::vec3 forward;
    forward.x = cos(yawRad) * cos(pitchRad);
    forward.y = sin(pitchRad);
    forward.z = sin(yawRad) * cos(pitchRad);
    forward = glm::normalize(forward);

    // Target position in front of camera
    glm::vec3 targetPos = camera_.GetPosition() + forward * grabDistance_;

    // Get current object position
    btTransform transform;
    grabbedBody_->getMotionState()->getWorldTransform(transform);
    btVector3 currentPos = transform.getOrigin();
    glm::vec3 objPos(currentPos.x(), currentPos.y(), currentPos.z());

    // Calculate velocity to move towards target (spring-like behavior)
    glm::vec3 delta = targetPos - objPos;
    float distance = glm::length(delta);
    
    // If object is too far, drop it
    if (distance > grabMaxDistance_ * 1.5f) {
        grabbedBody_->setGravity(btVector3(savedGravity_.x, savedGravity_.y, savedGravity_.z));
        grabbedBody_->activate();
        SE_LOG_INFO("Object dropped (too far)");
        grabbedBody_ = nullptr;
        return;
    }

    // Apply velocity towards target position
    float grabStrength = 15.0f;
    glm::vec3 velocity = delta * grabStrength;
    
    // Dampen existing velocity for smooth movement
    btVector3 currentVel = grabbedBody_->getLinearVelocity();
    glm::vec3 dampedVel = glm::vec3(currentVel.x(), currentVel.y(), currentVel.z()) * 0.5f;
    velocity = velocity + dampedVel * 0.1f;
    
    grabbedBody_->setLinearVelocity(btVector3(velocity.x, velocity.y, velocity.z));
    
    // Dampen angular velocity
    grabbedBody_->setAngularVelocity(grabbedBody_->getAngularVelocity() * 0.9f);
}

void ThirdPersonLayer::OnRender() {
    auto& window      = Application::Get().GetWindow();
    float aspectRatio = (float)window.GetWidth() / (float)window.GetHeight();
    
    // Render scene (entities with built-in frustum and occlusion culling)
    scene_->OnRender(camera_, aspectRatio);
    
    // Update and render debug drawing
    if (scene_->GetPhysicsSystem()) {
        scene_->GetPhysicsSystem()->UpdateDebugDraw(1.0f / 60.0f);
        scene_->GetPhysicsSystem()->RenderDebug(camera_);
    }
}

void ThirdPersonLayer::OnImGuiRender() {
    ImGui::Begin("Third Person Debug");

    if (playerEntity_.HasComponent<TransformComponent>()) {
        auto& trans = playerEntity_.GetComponent<TransformComponent>();
        ImGui::Text("Player Pos: %.2f, %.2f, %.2f", trans.Position.x, trans.Position.y, trans.Position.z);
    }

    ImGui::Text("Grounded: %s", isGrounded_ ? "Yes" : "No");
    ImGui::Text("Velocity Y: %.2f", playerVelocity_.y);
    ImGui::Text("Active Bullets: %zu", bullets_.size());

    if (playerEntity_.HasComponent<SpringArmComponent>()) {
        auto& springArm = playerEntity_.GetComponent<SpringArmComponent>();
        ImGui::DragFloat("Arm Length", &springArm.TargetArmLength, 0.1f, 1.0f, 20.0f);
        ImGui::DragFloat("Arm Pitch", &springArm.Pitch, 1.0f);
        ImGui::DragFloat("Arm Yaw", &springArm.Yaw, 1.0f);
        ImGui::DragFloat3("Socket Offset", &springArm.SocketOffset.x, 0.1f);
    }

    ImGui::Separator();
    ImGui::Text("Rotation Debug:");
    ImGui::Text("Target Yaw: %.2f", debugTargetYaw_);
    ImGui::Text("Current Yaw: %.2f", debugCurrentYaw_);
    ImGui::Text("New Yaw: %.2f", debugNewYaw_);

    ImGui::End();

    // Culling Debug Panel
    ImGui::Begin("Culling System");
    
    auto& sceneRenderer = Application::Get().GetRenderer().GetSceneRenderer();
    auto stats = sceneRenderer.GetStats();
    
    if (ImGui::Checkbox("Enable Frustum Culling", &enableFrustumCulling_)) {
        sceneRenderer.SetFrustumCullingEnabled(enableFrustumCulling_);
    }
    if (ImGui::Checkbox("Enable Occlusion Culling", &enableOcclusionCulling_)) {
        sceneRenderer.SetOcclusionCullingEnabled(enableOcclusionCulling_);
    }
    
    ImGui::Separator();
    ImGui::Text("Culling Statistics:");
    ImGui::Text("Total Objects: %u", stats.TotalObjects);
    ImGui::Text("Frustum Culled: %u", stats.FrustumCulled);
    ImGui::Text("Occlusion Culled: %u", stats.OcclusionCulled);
    ImGui::Text("Visible Objects: %u", stats.VisibleObjects);
    ImGui::Text("Draw Calls: %u", stats.DrawCalls);
    ImGui::Text("Triangles: %u", stats.TriangleCount);
    
    if (stats.TotalObjects > 0) {
        float cullRatio = static_cast<float>(stats.FrustumCulled + stats.OcclusionCulled) / stats.TotalObjects;
        ImGui::ProgressBar(cullRatio, ImVec2(-1, 0), "Total Cull Ratio");
    }
    
    ImGui::Separator();
    ImGui::Text("Bullet Cubes in Scene: %zu", bullets_.size());
    ImGui::Text("(Bullets persist for physics testing)");
    
    ImGui::End();

    // Performance Stats Panel
    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    const float PAD = 10.0f;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->GetWorkPos();
    ImVec2 work_size = viewport->GetWorkSize();
    ImVec2 window_pos, window_pos_pivot;
    window_pos.x = work_pos.x + work_size.x - PAD;
    window_pos.y = work_pos.y + PAD;
    window_pos_pivot.x = 1.0f;
    window_pos_pivot.y = 0.0f;
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::SetNextWindowBgAlpha(0.35f);
    if (ImGui::Begin("Performance Stats", nullptr, window_flags)) {
        ImGui::Text("FPS: %.1f", io.Framerate);
        ImGui::Text("Frametime: %.3f ms", 1000.0f / io.Framerate);
        
        ImGui::Separator();
        ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "-- Physics --");
        
        if (scene_ && scene_->GetPhysicsSystem()) {
            auto* physics = scene_->GetPhysicsSystem();
            float physicsTime = physics->GetLastPhysicsExecutionTime();
            size_t totalBodies = physics->GetActiveBodyCount();
            size_t sleepingBodies = physics->GetSleepingBodyCount();
            size_t activeBodies = totalBodies - sleepingBodies;
            bool isIdle = physics->IsIdle();
            
            ImGui::Text("Simulation: %.3f ms", physicsTime);
            ImGui::Text("Bodies: %zu total", totalBodies);
            ImGui::Text("  Active: %zu", activeBodies);
            ImGui::Text("  Sleeping: %zu", sleepingBodies);
            
            if (isIdle) {
                ImGui::TextColored(ImVec4(0.2f, 1.0f, 0.2f, 1.0f), "Status: IDLE (low CPU)");
            } else {
                ImGui::TextColored(ImVec4(1.0f, 0.5f, 0.2f, 1.0f), "Status: ACTIVE");
            }
            
            ImGui::Text("Threads: %zu", physics->GetThreadPoolSize());
        }
        
        ImGui::Separator();
        ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "-- Rendering --");
        ImGui::Text("Visible: %u/%u", stats.VisibleObjects, stats.TotalObjects);
        ImGui::Text("Draw Calls: %u", stats.DrawCalls);
        ImGui::Text("Triangles: %u", stats.TriangleCount);
        ImGui::Text("Bullets: %zu", bullets_.size());
    }
    ImGui::End();
}

