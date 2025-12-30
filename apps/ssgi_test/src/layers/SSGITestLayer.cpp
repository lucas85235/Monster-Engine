#include "SSGITestLayer.h"

#include "engine/Application.h"
#include "engine/Camera.h"
#include "engine/Log.h"
#include "engine/Renderer.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/input/InputManager.h"
#include "engine/renderer/Material.h"
#include "engine/renderer/SceneRenderer.h"
#include "engine/renderer/PBRMaterial.h"
#include "engine/renderer/IBLProcessor.h"
#include "engine/resources/MaterialManager.h"
#include "engine/resources/MeshManager.h"
#include "engine/resources/MapLoader.h"

#include <imgui.h>
#include <glm.hpp>
#include <gtc/matrix_transform.hpp>

namespace SSGITest {

SSGITestLayer::~SSGITestLayer() {
    auto& renderer = se::Application::Get().GetRenderer().GetSceneRenderer();
    renderer.ClearGlobalMaterialOverride();
}

void SSGITestLayer::OnAttach() {
    Layer::OnAttach();
    
    scene_ = se::CreateScope<se::Scene>("PBR Test Scene", se::SceneSettings{.EnablePhysics = false});
    se::Application::Get().SetActiveScene(scene_.get());
    
    camera_ = std::make_unique<Camera>(glm::vec3(0.0f, 5.0f, 15.0f));
    scene_->SetActiveCamera(camera_.get());
    
    CreateMaterials();
    SetupScene();
    
    auto& renderer = se::Application::Get().GetRenderer().GetSceneRenderer();
    renderer.SetRadianceCascadesEnabled(false);
    renderer.SetSparseRCEnabled(false);
    renderer.SetSSGIEnabled(false);
    
    // Find existing DirectionalLightComponent entity from map, or create one
    auto lightView = scene_->GetAllEntitiesWith<se::TransformComponent, se::DirectionalLightComponent>();
    for (auto entity : lightView) {
        lightEntity_ = se::Entity(entity, scene_.get());
        SE_LOG_INFO("[PBRTest] Found existing directional light entity");
        break;
    }
    
    if (!lightEntity_.IsValid()) {
        // Create directional light entity like ThirdPersonLayer
        lightEntity_ = scene_->CreateEntity("Sun");
        auto& transform = lightEntity_.GetComponent<se::TransformComponent>();
        transform.SetPosition({10.0f, 20.0f, 10.0f});
        transform.SetRotation({45.0f, 45.0f, 0.0f});
        
        auto& light = lightEntity_.AddComponent<se::DirectionalLightComponent>();
        light.Color = {1.0f, 0.95f, 0.9f};
        light.Intensity = 2.0f;
        light.CastShadows = true;
        light.Enabled = true;
        SE_LOG_INFO("[PBRTest] Created new directional light entity");
    }
    
    // Set up HDR IBL environment lighting from cubemap
    se::IBLData ibl;
    ibl.SetDefaultOutdoor();  // SH fallback
    
    // Try to load HDR environment map
    std::filesystem::path hdrPath = "assets/textures/ibl/lilienstein_4k.hdr";
    if (std::filesystem::exists(hdrPath)) {
        auto iblResult = se::IBLProcessor::ProcessHDR(hdrPath, 2048);
        if (iblResult.Valid) {
            ibl.EnvironmentCubemap = iblResult.EnvironmentCubemap;
            ibl.EnvironmentCubemapSize = iblResult.CubemapSize;
            ibl.IrradianceCubemap = iblResult.IrradianceCubemap;
            ibl.PrefilteredCubemap = iblResult.PrefilteredCubemap;
            ibl.DfgLut = iblResult.DfgLut;
            ibl.PrefilteredMipLevels = iblResult.PrefilteredMipLevels;
            ibl.Intensity = 1.0f;
            SE_LOG_INFO("[PBRTest] Loaded HDR IBL: {}", hdrPath.filename().string());
        } else {
            SE_LOG_WARN("[PBRTest] Failed to process HDR, using SH fallback");
        }
    } else {
        SE_LOG_WARN("[PBRTest] HDR file not found: {}, using SH fallback", hdrPath.string());
    }
    renderer.SetEnvironmentLighting(ibl);
    
    SE_LOG_INFO("[PBRTest] Test layer initialized with Material Override system");
}

void SSGITestLayer::OnDetach() {
    auto& renderer = se::Application::Get().GetRenderer().GetSceneRenderer();
    renderer.ClearGlobalMaterialOverride();
    se::Application::Get().SetActiveScene(nullptr);
    Layer::OnDetach();
}

void SSGITestLayer::CreateMaterials() {
    defaultMaterial_ = se::MaterialManager::GetDefaultMaterial();
    
    if (!defaultMaterial_) {
        SE_LOG_ERROR("[PBRTest] Failed to get default material!");
    }
}

void SSGITestLayer::SetupScene() {
    auto mapResult = se::MapLoader::Load(*scene_, "assets/maps/test.mstmap");
    if (!mapResult.success) {
        SE_LOG_ERROR("[PBRTest] Failed to load map: {}", mapResult.entityCount);
        return;
    }
    
    SE_LOG_INFO("[PBRTest] Loaded map with {} entities", scene_->GetEntityCount());
}

void SSGITestLayer::OnUpdate(float ts) {
    Layer::OnUpdate(ts);
    
    auto& input = se::InputManager::Get();
    
    if (input.IsMouseButtonDown(1)) {
        se::Vector2 delta = input.GetMouseDelta();
        cameraYaw_ += delta.x * 0.2f;
        cameraPitch_ -= delta.y * 0.2f;
        cameraPitch_ = glm::clamp(cameraPitch_, -89.0f, 89.0f);
        
        camera_->SetYaw(cameraYaw_);
        camera_->SetPitch(cameraPitch_);
    }
    
    if (input.IsKeyDown(se::Key::W)) {
        camera_->ProcessKeyboard(Camera::CameraMovement::FORWARD, camera_speed_ * ts);
    }
    if (input.IsKeyDown(se::Key::S)) {
        camera_->ProcessKeyboard(Camera::CameraMovement::BACKWARD, camera_speed_ * ts);
    }
    if (input.IsKeyDown(se::Key::A)) {
        camera_->ProcessKeyboard(Camera::CameraMovement::LEFT, camera_speed_ * ts);
    }
    if (input.IsKeyDown(se::Key::D)) {
        camera_->ProcessKeyboard(Camera::CameraMovement::RIGHT, camera_speed_ * ts);
    }
    if (input.IsKeyDown(se::Key::Q)) {
        camera_->ProcessKeyboard(Camera::CameraMovement::DOWN, camera_speed_ * ts);
    }
    if (input.IsKeyDown(se::Key::E)) {
        camera_->ProcessKeyboard(Camera::CameraMovement::UP, camera_speed_ * ts);
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
    
    ImGui::Begin("PBR Debug Panel");
    
    ImGui::Text("FPS: %.1f", ImGui::GetIO().Framerate);
    ImGui::Text("Resolution: %dx%d", window.GetWidth(), window.GetHeight());
    ImGui::Separator();
    
    ImGui::Text("Camera Controls:");
    ImGui::BulletText("Right-Click + Mouse: Rotate");
    ImGui::BulletText("WASD/QE: Move");
    ImGui::Separator();
    
    // Material Override Toggle
    static bool overrideEnabled = false;
    if (ImGui::Checkbox("Enable Material Override", &overrideEnabled)) {
        if (overrideEnabled) {
            renderer.SetGlobalMaterialOverride(&testMaterialParams_);
            SE_LOG_INFO("[PBRTest] Material override ENABLED");
        } else {
            renderer.ClearGlobalMaterialOverride();
            SE_LOG_INFO("[PBRTest] Material override DISABLED");
        }
    }
    ImGui::SameLine();
    ImGui::TextColored(overrideEnabled ? ImVec4(0,1,0,1) : ImVec4(1,0.5f,0,1), 
                       overrideEnabled ? "ACTIVE" : "INACTIVE");
    
    if (!overrideEnabled) {
        ImGui::TextWrapped("Enable override to edit material properties on all objects.");
    }
    
    ImGui::Separator();
    
    // Core PBR Parameters
    ImGui::Text("Core PBR:");
    ImGui::ColorEdit4("Base Color", &testMaterialParams_.BaseColor.x);
    ImGui::SliderFloat("Metallic", &testMaterialParams_.Metallic, 0.0f, 1.0f);
    ImGui::SliderFloat("Roughness", &testMaterialParams_.Roughness, 0.0f, 1.0f);
    ImGui::SliderFloat("Reflectance", &testMaterialParams_.Reflectance, 0.0f, 1.0f);
    ImGui::SliderFloat("AO", &testMaterialParams_.AO, 0.0f, 1.0f);
    
    ImGui::Separator();
    ImGui::Text("Clear Coat:");
    ImGui::SliderFloat("Clear Coat", &testMaterialParams_.ClearCoat, 0.0f, 1.0f);
    ImGui::SliderFloat("CC Roughness", &testMaterialParams_.ClearCoatRoughness, 0.0f, 1.0f);
    
    ImGui::Separator();
    ImGui::Text("Anisotropy:");
    ImGui::SliderFloat("Anisotropy", &testMaterialParams_.Anisotropy, -1.0f, 1.0f);
    
    ImGui::Separator();
    ImGui::Text("Sheen (Fabric):");
    ImGui::ColorEdit3("Sheen Color", &testMaterialParams_.SheenColor.x);
    ImGui::SliderFloat("Sheen Roughness", &testMaterialParams_.SheenRoughness, 0.0f, 1.0f);
    
    ImGui::Separator();
    ImGui::Text("Subsurface:");
    ImGui::ColorEdit3("Subsurface Color", &testMaterialParams_.SubsurfaceColor.x);
    ImGui::SliderFloat("Subsurface Power", &testMaterialParams_.SubsurfacePower, 0.0f, 10.0f);
    ImGui::SliderFloat("Thickness", &testMaterialParams_.Thickness, 0.0f, 1.0f);
    
    ImGui::Separator();
    ImGui::Text("Transmission:");
    ImGui::SliderFloat("Transmission", &testMaterialParams_.Transmission, 0.0f, 1.0f);
    ImGui::SliderFloat("IOR", &testMaterialParams_.IOR, 1.0f, 3.0f);
    
    ImGui::Separator();
    ImGui::Text("Emissive:");
    ImGui::ColorEdit3("Emissive Color", &testMaterialParams_.EmissiveColor.x);
    ImGui::SliderFloat("Emissive Factor", &testMaterialParams_.EmissiveFactor, 0.0f, 10.0f);
    
    ImGui::Separator();
    
    // Lighting controls - modify the light entity's components
    ImGui::Text("Directional Light:");
    
    // Initialize static values from the entity on first frame
    static float lightAzimuth = 210.0f;
    static float lightElevation = 45.0f;
    static bool firstFrame = true;
    
    if (lightEntity_.IsValid() && lightEntity_.HasComponent<se::DirectionalLightComponent>()) {
        auto& light = lightEntity_.GetComponent<se::DirectionalLightComponent>();
        auto& transform = lightEntity_.GetComponent<se::TransformComponent>();
        
        // On first frame, derive azimuth/elevation from entity's rotation
        if (firstFrame) {
            se::Vector3 rotation = transform.Rotation;
            // rotation.x > 0 means light points down, so elevation = rotation.x
            lightElevation = rotation.x;
            lightAzimuth = rotation.y + 180.0f;
        }
        
        bool lightChanged = false;
        lightChanged |= ImGui::SliderFloat("Azimuth", &lightAzimuth, 0.0f, 360.0f, "%.1f deg");
        lightChanged |= ImGui::SliderFloat("Elevation", &lightElevation, 5.0f, 90.0f, "%.1f deg");
        lightChanged |= ImGui::ColorEdit3("Light Color", &light.Color.x);
        lightChanged |= ImGui::SliderFloat("Intensity", &light.Intensity, 0.0f, 10.0f);
        
        if (lightChanged) {
            // Positive elevation -> positive rotation.x tilts forward vector down
            // RenderSystem uses -transform.GetForward() for direction FROM light TO scene
            transform.SetRotation({lightElevation, lightAzimuth - 180.0f, 0.0f});
        }
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "No light entity found!");
    }
    
    ImGui::Separator();
    
    // CSM Debug Controls
    ImGui::Text("Shadow Cascades (CSM):");
    
    static bool csmEnabled = true;
    if (firstFrame) renderer.SetCSMEnabled(csmEnabled);
    if (ImGui::Checkbox("Enable CSM", &csmEnabled)) {
        renderer.SetCSMEnabled(csmEnabled);
    }
    
    static bool visualizeCascades = false;  // Disabled by default for cleaner view
    if (firstFrame) renderer.SetVisualizeCascades(visualizeCascades);
    if (ImGui::Checkbox("Visualize Cascades", &visualizeCascades)) {
        renderer.SetVisualizeCascades(visualizeCascades);
    }
    
    static float splitLambda = 0.85f;
    if (firstFrame) renderer.SetCSMSplitLambda(splitLambda);
    if (ImGui::SliderFloat("Split Lambda", &splitLambda, 0.0f, 1.0f, "%.2f")) {
        renderer.SetCSMSplitLambda(splitLambda);
    }
    ImGui::TextWrapped("0 = uniform splits, 1 = logarithmic (more detail near camera)");
    
    firstFrame = false;
    
    ImGui::Separator();
    
    // Presets
    ImGui::Text("Material Presets:");
    if (ImGui::Button("Gold")) {
        testMaterialParams_.BaseColor = se::Vector4(1.0f, 0.765f, 0.336f, 1.0f);
        testMaterialParams_.Metallic = 1.0f;
        testMaterialParams_.Roughness = 0.3f;
        testMaterialParams_.ClearCoat = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Silver")) {
        testMaterialParams_.BaseColor = se::Vector4(0.972f, 0.960f, 0.915f, 1.0f);
        testMaterialParams_.Metallic = 1.0f;
        testMaterialParams_.Roughness = 0.1f;
        testMaterialParams_.ClearCoat = 0.0f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Car Paint")) {
        testMaterialParams_.BaseColor = se::Vector4(0.8f, 0.1f, 0.1f, 1.0f);
        testMaterialParams_.Metallic = 0.0f;
        testMaterialParams_.Roughness = 0.5f;
        testMaterialParams_.ClearCoat = 1.0f;
        testMaterialParams_.ClearCoatRoughness = 0.1f;
    }
    
    if (ImGui::Button("Fabric")) {
        testMaterialParams_.BaseColor = se::Vector4(0.2f, 0.3f, 0.6f, 1.0f);
        testMaterialParams_.Metallic = 0.0f;
        testMaterialParams_.Roughness = 0.8f;
        testMaterialParams_.SheenColor = se::Vector3(0.5f, 0.6f, 0.8f);
        testMaterialParams_.SheenRoughness = 0.5f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Skin")) {
        testMaterialParams_.BaseColor = se::Vector4(0.8f, 0.6f, 0.5f, 1.0f);
        testMaterialParams_.Metallic = 0.0f;
        testMaterialParams_.Roughness = 0.5f;
        testMaterialParams_.SubsurfaceColor = se::Vector3(1.0f, 0.2f, 0.1f);
        testMaterialParams_.SubsurfacePower = 2.0f;
        testMaterialParams_.Thickness = 0.5f;
    }
    ImGui::SameLine();
    if (ImGui::Button("Plastic")) {
        testMaterialParams_.BaseColor = se::Vector4(0.1f, 0.3f, 0.8f, 1.0f);
        testMaterialParams_.Metallic = 0.0f;
        testMaterialParams_.Roughness = 0.4f;
        testMaterialParams_.ClearCoat = 0.0f;
    }
    
    ImGui::Separator();
    ImGui::Text("Scene Info:");
    ImGui::Text("Entities: %d", (int)scene_->GetEntityCount());
    
    ImGui::End();
}

} // namespace SSGITest
