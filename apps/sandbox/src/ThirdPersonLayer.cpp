#include "ThirdPersonLayer.h"

#include <engine/Application.h>
#include <engine/Log.h>
#include <engine/input/InputManager.h>
#include <engine/resources/MeshManager.h>
#include <engine/ecs/SimpleComponents.h>
#include <imgui.h>
#include <gtc/matrix_transform.hpp>

#include "SampleUtilities.h"
#include "MathUtils.h"

ThirdPersonLayer::ThirdPersonLayer() 
    : Layer("ThirdPersonLayer"), 
      camera_(glm::vec3(0.0f, 5.0f, 10.0f)) {
}

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
    auto& app = Application::Get();
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
        auto floor = scene_->CreateEntity("Floor");
        auto mesh = MeshManager::GetPrimitive(PrimitiveMeshType::Cube);
        floor.AddComponent<MeshRenderComponent>(mesh, material_);
        
        auto& transform = floor.GetComponent<TransformComponent>();
        transform.SetPosition({0.0f, -1.0f, 0.0f});
        transform.SetScale({50.0f, 2.0f, 50.0f});
    }

    // Create player cube
    {
        playerEntity_ = scene_->CreateEntity("Player");
        auto mesh = MeshManager::GetPrimitive(PrimitiveMeshType::Cube);
        
        if (!material_) {
            SE_LOG_ERROR("Material not loaded, retrying");
            material_ = Utilities::LoadMaterial();
        }
        
        playerEntity_.AddComponent<MeshRenderComponent>(mesh, material_);
        
        auto& transform = playerEntity_.GetComponent<TransformComponent>();
        transform.SetPosition({0.0f, 2.0f, 0.0f});
        transform.SetScale({1.0f, 1.0f, 1.0f});
        
        // Add camera spring arm
        auto& springArm = playerEntity_.AddComponent<SpringArmComponent>();
        springArm.TargetArmLength = 8.0f;
        springArm.SocketOffset = {0.0f, 1.5f, 0.0f};
        springArm.Pitch = -30.0f;
    }

    // Create directional light
    {
        auto light = scene_->CreateEntity("Sun");
        auto& transform = light.GetComponent<TransformComponent>();
        transform.SetPosition({10.0f, 20.0f, 10.0f});
        transform.SetRotation({45.0f, 45.0f, 0.0f});
        
        auto& dirLight = light.AddComponent<DirectionalLightComponent>();
        dirLight.Color = {1.0f, 0.9f, 0.8f};
        dirLight.Intensity = 1.2f;
        dirLight.CastShadows = true;
    }
}

void ThirdPersonLayer::OnUpdate(float ts) {
    UpdatePlayer(ts);
    UpdateCamera();
    scene_->OnUpdate(ts);
}

void ThirdPersonLayer::UpdatePlayer(float ts) {
    auto& input = InputManager::Get();
    auto& transform = playerEntity_.GetComponent<TransformComponent>();
    auto& springArm = playerEntity_.GetComponent<SpringArmComponent>();
    
    glm::vec3 position = transform.Position;

    // Calculate camera-relative movement direction
    float moveForward = input.GetAxis("MoveForward");
    float moveRight = input.GetAxis("MoveRight");

    float yawRad = springArm.Yaw * 0.0174533f;
    float sinYaw = std::sin(yawRad);
    float cosYaw = std::cos(yawRad);
    
    glm::vec3 camForward = {-sinYaw, 0.0f, -cosYaw};
    glm::vec3 camRight = {cosYaw, 0.0f, -sinYaw};

    glm::vec3 movement = (camForward * moveForward + camRight * moveRight);
    
    if (glm::length(movement) > 0.01f) {
        movement = glm::normalize(movement);
        
        // Smoothly rotate player to face movement direction
        float targetYaw = Math::CalculateYawFromDirection(movement.x, movement.z);
        float currentYaw = transform.Rotation.y;
        float rotationSpeed = 10.0f;
        float newYaw = Math::InterpolateYaw(currentYaw, targetYaw, rotationSpeed * ts);
        
        transform.Rotation.y = newYaw;
        position += movement * moveSpeed_ * ts;
    }

    // Handle jumping and gravity
    if (input.IsActionJustPressed("Jump") && isGrounded_) {
        playerVelocity_.y = jumpForce_;
        isGrounded_ = false;
    }

    playerVelocity_.y -= gravity_ * ts;
    position.y += playerVelocity_.y * ts;

    // Simple floor collision (y = 0)
    float playerBottom = position.y - 0.5f;
    float floorY = 0.0f;

    if (playerBottom <= floorY) {
        position.y = floorY + 0.5f;
        playerVelocity_.y = 0.0f;
        isGrounded_ = true;
    } else {
        isGrounded_ = false;
    }

    transform.SetPosition(position);
}

void ThirdPersonLayer::UpdateCamera() {
    auto& input = InputManager::Get();
    
    if (!playerEntity_.HasComponent<SpringArmComponent>()) return;
    auto& springArm = playerEntity_.GetComponent<SpringArmComponent>();
    auto& playerTrans = playerEntity_.GetComponent<TransformComponent>();

    // Update spring arm rotation from mouse input
    float mouseX = input.GetAxis("CameraRotateX");
    float mouseY = input.GetAxis("CameraRotateY");
    
    springArm.Yaw -= mouseX * 0.1f;
    springArm.Pitch -= mouseY * 0.1f;
    springArm.Pitch = glm::clamp(springArm.Pitch, springArm.MinPitch, springArm.MaxPitch);

    // Calculate camera position using spherical coordinates
    float yawRad = springArm.Yaw * 0.0174533f;
    float pitchRad = springArm.Pitch * 0.0174533f;
    
    float sinYaw = std::sin(yawRad);
    float cosYaw = std::cos(yawRad);
    float sinPitch = std::sin(pitchRad);
    float cosPitch = std::cos(pitchRad);
    
    glm::vec3 camOffset;
    camOffset.x = springArm.TargetArmLength * cosPitch * sinYaw;
    camOffset.y = springArm.TargetArmLength * sinPitch;
    camOffset.z = springArm.TargetArmLength * cosPitch * cosYaw;
    
    glm::vec3 targetPos = playerTrans.Position + springArm.SocketOffset;
    glm::vec3 camPos = targetPos + camOffset;

    camera_.SetPosition(camPos);
    
    // Set camera rotation to look at target
    float camYaw = -springArm.Yaw - 90.0f;
    float camPitch = -springArm.Pitch;

    camera_.SetYaw(camYaw);
    camera_.SetPitch(camPitch);
}

void ThirdPersonLayer::OnRender() {
    auto& window = Application::Get().GetWindow();
    float aspectRatio = (float)window.GetWidth() / (float)window.GetHeight();
    scene_->OnRender(camera_, aspectRatio);
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
    
    ImGui::End();
}
