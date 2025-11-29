#include "ThirdPersonLayer.h"

#include <engine/Application.h>
#include <engine/Log.h>
#include <engine/ecs/SimpleComponents.h>
#include <engine/input/InputManager.h>
#include <engine/resources/MeshManager.h>
#include <imgui.h>

#include <gtc/matrix_transform.hpp>

#include "MathUtils.h"
#include "SampleUtilities.h"

ThirdPersonLayer::ThirdPersonLayer() : Layer("ThirdPersonLayer"), camera_(glm::vec3(0.0f, 5.0f, 10.0f)) {}

ThirdPersonLayer::~ThirdPersonLayer() {}

void ThirdPersonLayer::OnAttach() {
    SE_LOG_INFO("ThirdPersonLayer attached");

    material_ = Utilities::LoadMaterial();
    CreateScene();

    // Bind input axes and actions
    auto& input = InputManager::Get();
    input.BindAxis("MoveForward", Key::W, 1.0f);
    input.BindAxis("MoveForward", Key::S, -1.0f);
    input.BindAxis("MoveRight", Key::D, 1.0f);
    input.BindAxis("MoveRight", Key::A, -1.0f);
    input.BindAction("Jump", Key::Space);
    input.BindAxis("CameraRotateX", Key::MouseX, 1.0f);
    input.BindAxis("CameraRotateY", Key::MouseY, -1.0f);

    // Capture and hide cursor
    auto& app    = Application::Get();
    auto* window = app.GetWindow().GetNativeWindow();
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
}

void ThirdPersonLayer::OnDetach() {
    scene_.reset();
}

void ThirdPersonLayer::CreateScene() {
    scene_ = CreateScope<Scene>("Third Person Scene");

    // Create floor
    {
        floor_entity_ = scene_->CreateEntity("Floor");
        auto mesh     = MeshManager::GetPrimitive(PrimitiveMeshType::Cube);
        floor_entity_.AddComponent<MeshRenderComponent>(mesh, material_);

        auto& transform = floor_entity_.GetComponent<TransformComponent>();
        transform.SetPosition({0.0f, -1.0f, 0.0f});
        transform.SetScale({50.0f, 2.0f, 50.0f});

        RigidbodyData data = RigidbodyData{.mass = 0.0f, .gravityScale = 1.0f};
        floor_entity_.AddComponent<RigidbodyComponent>(data, floor_entity_);
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
}

void ThirdPersonLayer::OnUpdate(float ts) {
    UpdatePlayer(ts);
    UpdateCamera();
    scene_->OnUpdate(ts);
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

        // Debug capture
        debugTargetYaw_ = targetYaw;
        debugCurrentYaw_ = currentYaw;
        debugNewYaw_ = newYaw;

        // Check for large jumps
        if (std::abs(newYaw - currentYaw) > 10.0f && std::abs(newYaw - currentYaw) < 350.0f) {
             SE_LOG_WARN("Rotation Jump Detected! Current: {}, Target: {}, New: {}", currentYaw, targetYaw, newYaw);
        }

        // Sync rotation to physics body
        btTransform tr = rb.GetRigidbody()->getWorldTransform();
        
        // Debug physics yaw before set
        btQuaternion currentQ = tr.getRotation();
        glm::quat qNorm(currentQ.w(), currentQ.x(), currentQ.y(), currentQ.z());
        qNorm = glm::normalize(qNorm);
        debugPhysicsYaw_ = glm::degrees(glm::yaw(qNorm));
        debugPitch_ = glm::degrees(glm::pitch(qNorm));
        debugRoll_ = glm::degrees(glm::roll(qNorm));

        btQuaternion q;
        q.setEuler(0, glm::radians(newYaw), 0); 
        
        glm::quat glmQ = glm::quat(glm::vec3(0, glm::radians(newYaw), 0));
        tr.setRotation(btQuaternion(glmQ.x, glmQ.y, glmQ.z, glmQ.w));
        rb.GetRigidbody()->setWorldTransform(tr);

        desiredVelocity.setX(movement.x * moveSpeed_);
        desiredVelocity.setZ(movement.z * moveSpeed_);
    } else {
        // Stop horizontal movement
        desiredVelocity.setX(0);
        desiredVelocity.setZ(0);
    }

    rb.SetLinearVelocity(desiredVelocity);

    // Handle jumping
    if (input.IsActionJustPressed("Jump") && isGrounded_) {
        isGrounded_ = false;
        rb.AddImpulse({0.0f, 100.0f, 0.0f}, {0.0f, 0.0f, 0.0f}); // Impulse at center
    }

    // Simple floor check using raycast or just position (temporary)
    // Ideally we should use collision callbacks or a raycast.
    // For now, let's trust the physics engine to handle collision, 
    // but we need isGrounded for jumping.
    // Let's use a simple raycast or check velocity Y approx 0?
    // Checking velocity is unreliable.
    // Let's assume grounded if y velocity is near 0 and we are low enough?
    // Or just allow jumping for now.
    // Let's keep the old simple check but based on physics position?
    // No, let's use the contact manifold in a real engine, but here we don't have it exposed easily yet.
    // Let's just allow infinite jump for debug or a simple height check.
    // Player capsule has total height 2.0 (radius 0.5, height 1.0). Center is at 1.0 from bottom.
    // Floor is at 0.0. So player center should be at 1.0.
    // We check if we are close to the ground and not moving up/down significantly.
    if (transform.Position.y < 1.1f && std::abs(rb.GetLinearVelocity().y()) < 0.1f) {
        isGrounded_ = true;
    } else {
        isGrounded_ = false;
    }
}


void ThirdPersonLayer::UpdateCamera() {
    auto& input = InputManager::Get();

    if (!playerEntity_.HasComponent<SpringArmComponent>()) return;
    auto& springArm   = playerEntity_.GetComponent<SpringArmComponent>();
    auto& playerTrans = playerEntity_.GetComponent<TransformComponent>();

    // Update spring arm rotation from mouse input
    float mouseX = input.GetAxis("CameraRotateX");
    float mouseY = input.GetAxis("CameraRotateY");

    springArm.Yaw -= mouseX * 0.1f;
    springArm.Pitch -= mouseY * 0.1f;
    springArm.Pitch = glm::clamp(springArm.Pitch, springArm.MinPitch, springArm.MaxPitch);

    // Calculate camera position using spherical coordinates
    float yawRad   = springArm.Yaw * 0.0174533f;
    float pitchRad = springArm.Pitch * 0.0174533f;

    float sinYaw   = std::sin(yawRad);
    float cosYaw   = std::cos(yawRad);
    float sinPitch = std::sin(pitchRad);
    float cosPitch = std::cos(pitchRad);

    glm::vec3 camOffset;
    camOffset.x = springArm.TargetArmLength * cosPitch * sinYaw;
    camOffset.y = springArm.TargetArmLength * sinPitch;
    camOffset.z = springArm.TargetArmLength * cosPitch * cosYaw;

    glm::vec3 targetPos = playerTrans.Position + springArm.SocketOffset;
    glm::vec3 camPos    = targetPos + camOffset;

    camera_.SetPosition(camPos);

    // Set camera rotation to look at target
    float camYaw   = -springArm.Yaw - 90.0f;
    float camPitch = -springArm.Pitch;

    camera_.SetYaw(camYaw);
    camera_.SetPitch(camPitch);
}

void ThirdPersonLayer::OnRender() {
    auto& window      = Application::Get().GetWindow();
    float aspectRatio = (float)window.GetWidth() / (float)window.GetHeight();
    scene_->OnRender(camera_, aspectRatio);
    Application::Get().GetPhysicsManager().RenderDebug(camera_);
}

void ThirdPersonLayer::OnImGuiRender() {
    ImGui::Begin("Third Person Debug");

    if (playerEntity_.HasComponent<TransformComponent>()) {
        auto& trans = playerEntity_.GetComponent<TransformComponent>();
        ImGui::Text("Player Pos: %.2f, %.2f, %.2f", trans.Position.x, trans.Position.y, trans.Position.z);
    }

    ImGui::Text("Grounded: %s", isGrounded_ ? "Yes" : "No");
    ImGui::Text("Velocity Y: %.2f", playerVelocity_.y);

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
    ImGui::Text("Physics Yaw: %.2f", debugPhysicsYaw_);
    ImGui::Text("Physics Pitch: %.2f", debugPitch_);
    ImGui::Text("Physics Roll: %.2f", debugRoll_);

    ImGui::End();
}
