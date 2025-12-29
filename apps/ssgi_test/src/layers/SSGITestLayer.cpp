#include "SSGITestLayer.h"

#include "engine/Application.h"
#include "engine/Camera.h"
#include "engine/Log.h"
#include "engine/Renderer.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/input/InputManager.h"
#include "engine/renderer/Material.h"
#include "engine/renderer/SceneRenderer.h"
#include "engine/renderer/SSGIPass.h"
#include "engine/resources/MaterialManager.h"
#include "engine/resources/MeshManager.h"

#include <imgui.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

namespace SSGITest {

SSGITestLayer::~SSGITestLayer() = default;

void SSGITestLayer::OnAttach() {
    Layer::OnAttach();
    
    scene_ = se::CreateScope<se::Scene>("SSGI Test Scene", se::SceneSettings{.EnablePhysics = false});
    se::Application::Get().SetActiveScene(scene_.get());
    
    // Create camera
    camera_ = std::make_unique<Camera>(glm::vec3(0.0f, 5.0f, 15.0f));
    scene_->SetActiveCamera(camera_.get());
    
    // Create default material
    CreateMaterials();
    
    // Setup scene with primitives
    SetupScene();
    
    // Configure scene renderer for SSGI
    auto& renderer = se::Application::Get().GetRenderer().GetSceneRenderer();
    renderer.SetRadianceCascadesEnabled(false);
    renderer.SetSparseRCEnabled(false);
    renderer.SetSSGIEnabled(true);
    
    // Configure directional light
    se::SceneRenderer::DirectionalLightData sunLight;
    sunLight.Direction = glm::normalize(glm::vec3(-0.5f, -1.0f, -0.3f));
    sunLight.Color = glm::vec3(1.0f, 0.95f, 0.9f);
    sunLight.Intensity = 1.5f;
    sunLight.Active = true;
    sunLight.CastShadows = true;
    renderer.SetDirectionalLight(sunLight);
    
    SE_LOG_INFO("[SSGITest] Test layer initialized");
}

void SSGITestLayer::OnDetach() {
    se::Application::Get().SetActiveScene(nullptr);
    Layer::OnDetach();
}

void SSGITestLayer::CreateMaterials() {
    // Get default shader from MaterialManager
    defaultMaterial_ = se::MaterialManager::GetDefaultMaterial();
    
    if (!defaultMaterial_) {
        SE_LOG_ERROR("[SSGITest] Failed to get default material!");
    }
}

void SSGITestLayer::SetupScene() {
    auto cubeMesh = se::MeshManager::GetPrimitive(se::PrimitiveMeshType::Cube);
    
    if (!cubeMesh) {
        SE_LOG_ERROR("[SSGITest] Failed to get cube primitive!");
        return;
    }
    
    // Create ground plane (large box)
    {
        auto ground = scene_->CreateEntity("Ground");
        auto& transform = ground.GetComponent<se::TransformComponent>();
        transform.SetPosition({0.0f, -0.5f, 0.0f});
        transform.SetScale({20.0f, 1.0f, 20.0f});
        
        auto& mesh = ground.AddComponent<se::MeshRenderComponent>(cubeMesh, se::CreateRef<se::Material>(*defaultMaterial_));
        mesh.Color = se::Vector4(0.9f, 0.9f, 0.9f, 1.0f);
    }
    
    // Back wall
    {
        auto wall = scene_->CreateEntity("BackWall");
        auto& transform = wall.GetComponent<se::TransformComponent>();
        transform.SetPosition({0.0f, 5.0f, -10.0f});
        transform.SetScale({20.0f, 10.0f, 1.0f});
        
        auto& mesh = wall.AddComponent<se::MeshRenderComponent>(cubeMesh, se::CreateRef<se::Material>(*defaultMaterial_));
        mesh.Color = se::Vector4(0.9f, 0.9f, 0.9f, 1.0f);
    }
    
    // Left wall (red for color bleeding)
    {
        auto wall = scene_->CreateEntity("LeftWall");
        auto& transform = wall.GetComponent<se::TransformComponent>();
        transform.SetPosition({-10.0f, 5.0f, 0.0f});
        transform.SetScale({1.0f, 10.0f, 20.0f});
        
        auto& mesh = wall.AddComponent<se::MeshRenderComponent>(cubeMesh, se::CreateRef<se::Material>(*defaultMaterial_));
        mesh.Color = se::Vector4(0.9f, 0.2f, 0.2f, 1.0f);
    }
    
    // Right wall (green for color bleeding)
    {
        auto wall = scene_->CreateEntity("RightWall");
        auto& transform = wall.GetComponent<se::TransformComponent>();
        transform.SetPosition({10.0f, 5.0f, 0.0f});
        transform.SetScale({1.0f, 10.0f, 20.0f});
        
        auto& mesh = wall.AddComponent<se::MeshRenderComponent>(cubeMesh, se::CreateRef<se::Material>(*defaultMaterial_));
        mesh.Color = se::Vector4(0.2f, 0.9f, 0.2f, 1.0f);
    }
    
    // Emissive cube (light source)
    {
        auto emissiveCube = scene_->CreateEntity("EmissiveCube");
        auto& transform = emissiveCube.GetComponent<se::TransformComponent>();
        transform.SetPosition({0.0f, 3.0f, -5.0f});
        transform.SetScale({2.0f, 2.0f, 2.0f});
        
        auto& mesh = emissiveCube.AddComponent<se::MeshRenderComponent>(cubeMesh, se::CreateRef<se::Material>(*defaultMaterial_));
        mesh.Color = se::Vector4(1.0f, 1.0f, 1.0f, 1.0f);
        mesh.EmissiveColor = glm::vec3(2.0f, 1.8f, 0.8f);
        mesh.EmissiveFactor = 5.0f;
    }
    
    // Central pillar
    {
        auto pillar = scene_->CreateEntity("Pillar");
        auto& transform = pillar.GetComponent<se::TransformComponent>();
        transform.SetPosition({0.0f, 2.0f, 3.0f});
        transform.SetScale({1.5f, 4.0f, 1.5f});
        
        auto& mesh = pillar.AddComponent<se::MeshRenderComponent>(cubeMesh, se::CreateRef<se::Material>(*defaultMaterial_));
        mesh.Color = se::Vector4(0.9f, 0.9f, 0.9f, 1.0f);
    }
    
    SE_LOG_INFO("[SSGITest] Scene setup complete with {} entities", scene_->GetEntityCount());
}

void SSGITestLayer::OnUpdate(float ts) {
    Layer::OnUpdate(ts);
    
    auto& input = se::InputManager::Get();
    
    // Camera rotation with right mouse button (1 = right button)
    if (input.IsMouseButtonDown(1)) {
        se::Vector2 delta = input.GetMouseDelta();
        cameraYaw_ += delta.x * 0.2f;
        cameraPitch_ -= delta.y * 0.2f;
        cameraPitch_ = glm::clamp(cameraPitch_, -89.0f, 89.0f);
        
        camera_->SetYaw(cameraYaw_);
        camera_->SetPitch(cameraPitch_);
    }
    
    // WASD movement
    float moveSpeed = 10.0f * ts;
    if (input.IsKeyDown(se::Key::W)) {
        camera_->ProcessKeyboard(Camera::CameraMovement::FORWARD, ts);
    }
    if (input.IsKeyDown(se::Key::S)) {
        camera_->ProcessKeyboard(Camera::CameraMovement::BACKWARD, ts);
    }
    if (input.IsKeyDown(se::Key::A)) {
        camera_->ProcessKeyboard(Camera::CameraMovement::LEFT, ts);
    }
    if (input.IsKeyDown(se::Key::D)) {
        camera_->ProcessKeyboard(Camera::CameraMovement::RIGHT, ts);
    }
    if (input.IsKeyDown(se::Key::Q)) {
        camera_->ProcessKeyboard(Camera::CameraMovement::DOWN, ts);
    }
    if (input.IsKeyDown(se::Key::E)) {
        camera_->ProcessKeyboard(Camera::CameraMovement::UP, ts);
    }
    
    scene_->OnUpdate(ts);
}

void SSGITestLayer::OnRender() {
    Layer::OnRender();
    scene_->OnRender();
}

void SSGITestLayer::OnImGuiRender() {
    Layer::OnImGuiRender();
    RenderDebugPanel();
}

void SSGITestLayer::RenderDebugPanel() {
    auto& app = se::Application::Get();
    auto& window = app.GetWindow();
    auto& renderer = app.GetRenderer().GetSceneRenderer();
    
    ImGui::Begin("SSGI Debug Panel");
    
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Resolution: %dx%d", window.GetWidth(), window.GetHeight());
    ImGui::Separator();
    
    ImGui::Text("Camera Controls:");
    ImGui::BulletText("Right-Click + Mouse: Rotate camera");
    ImGui::BulletText("WASD: Move camera");
    ImGui::BulletText("Q/E: Move up/down");
    ImGui::Separator();
    
    // SSGI controls
    ImGui::Text("SSGI Controls:");
    
    bool ssgiEnabled = renderer.IsSSGIEnabled();
    if (ImGui::Checkbox("Enable SSGI", &ssgiEnabled)) {
        renderer.SetSSGIEnabled(ssgiEnabled);
    }
    
    auto& config = renderer.GetSSGIConfig();
    
    ImGui::SliderFloat("Intensity", &config.Intensity, 0.0f, 5.0f);
    ImGui::SliderInt("Ray Count", &config.RayCount, 4, 32);
    ImGui::SliderInt("Steps/Ray", &config.StepsPerRay, 4, 32);
    ImGui::SliderFloat("Max Distance", &config.MaxDistance, 1.0f, 50.0f);
    ImGui::SliderFloat("Resolution Scale", &config.ResolutionScale, 0.25f, 1.0f);
    ImGui::SliderInt("Blur Radius", &config.BlurRadius, 0, 8);
    
    const char* debugModes[] = {"Off", "SH Coefficients", "Raw Radiance", "Normals"};
    ImGui::Combo("Debug Mode", &config.DebugMode, debugModes, 4);
    
    if (auto* ssgiPass = renderer.GetSSGIPass()) {
        ImGui::Text("Work Res: %dx%d", ssgiPass->GetWorkWidth(), ssgiPass->GetWorkHeight());
    }
    
    ImGui::Separator();
    ImGui::Text("Scene Info:");
    ImGui::Text("Entities: %d", (int)scene_->GetEntityCount());
    
    ImGui::End();
}

} // namespace SSGITest
