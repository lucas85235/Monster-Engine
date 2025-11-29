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
#include "engine/physics/PhysicsSystem.h"

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

    // Apply movement to desired velocity
    float speed = 5.0f;
    desiredVelocity.setX(movement.x * speed);
    desiredVelocity.setZ(movement.z * speed);

    // Set the velocity on the rigidbody
    rb.SetLinearVelocity(desiredVelocity);

    // Shooting
    if (input.IsMouseButtonDown(0)) { // Left Mouse Button
        Shoot();
    }
}

void ThirdPersonLayer::Shoot() {
    float time = (float)glfwGetTime();
    if (time - lastShootTime_ < 0.2f) return; // 0.2s cooldown
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
    glm::vec3 spawnPos = camera_.GetPosition() + forward * 2.0f;

    // Create Entity
    auto box = scene_->CreateEntity("BulletBox");
    auto mesh = MeshManager::GetPrimitive(PrimitiveMeshType::Cube);
    box.AddComponent<MeshRenderComponent>(mesh, material_);
    box.AddComponent<BoxCollider>(glm::vec3(0.5f)); 
    
    box.GetComponent<TransformComponent>().SetPosition(spawnPos);
    box.GetComponent<TransformComponent>().SetScale(glm::vec3(0.5f));

    RigidbodyData data;
    data.mass = 2.0f;
    auto& rb = box.AddComponent<RigidbodyComponent>(data, box);
    
    // Apply impulse
    btVector3 impulse(forward.x, forward.y, forward.z);
    impulse *= 50.0f; // Force
    rb.GetRigidbody()->applyCentralImpulse(impulse);
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
    // Application::Get().GetPhysicsManager().RenderDebug(camera_);
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

    // Performance Stats Panel
    ImGuiIO& io = ImGui::GetIO();
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    const float PAD = 10.0f;
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImVec2 work_pos = viewport->GetWorkPos(); // Use work area to avoid menu-bar/task-bar, if any!
    ImVec2 work_size = viewport->GetWorkSize();
    ImVec2 window_pos, window_pos_pivot;
    window_pos.x = work_pos.x + work_size.x - PAD;
    window_pos.y = work_pos.y + PAD;
    window_pos_pivot.x = 1.0f;
    window_pos_pivot.y = 0.0f;
    ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
    if (ImGui::Begin("Performance Stats", nullptr, window_flags)) {
        ImGui::Text("FPS: %.1f", io.Framerate);
        ImGui::Text("Frametime: %.3f ms", 1000.0f / io.Framerate);
        
        if (scene_ && scene_->GetPhysicsSystem()) {
            float physicsTime = scene_->GetPhysicsSystem()->GetLastPhysicsExecutionTime();
            ImGui::Text("Physics: %.3f ms", physicsTime);
        }
    }
    ImGui::End();
}
