#include "MainGameLayer.h"

#include "../../../SampleUtilities.h"
#include "../components/CameraController.h"
#include "../components/Character.h"
#include "../components/CharacterController.h"
#include "../components/CharacterRender.h"
#include "engine/Application.h"
#include "engine/Camera.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/input/InputManager.h"
#include "engine/physics/PhysicsDebugDraw.h"
#include "engine/physics/PhysicsSystem.h"
#include "engine/Renderer.h"
#include "engine/resources/MeshManager.h"
#include "engine/resources/MapLoader.h"
#include "engine/renderer/IBLProcessor.h"
#include <imgui.h>

#include <filesystem>


namespace FirstGame {

void MainGameLayer::ImguiDebug() {
    auto& app    = se::Application::Get();
    auto& window = app.GetWindow();

    ImGui::Begin("Main Game Debug");
    ImGui::Text("Debug Info");
    ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
    ImGui::Text("Current Resolution: [%d x %d]", window.GetWidth(), window.GetHeight());
    ImGui::Separator();
    ImGui::Text("Press TAB to toggle mouse capture");
    

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
    }

    ImGui::End();
}

MainGameLayer::~MainGameLayer() = default;

void MainGameLayer::OnAttach() {
    Layer::OnAttach();
    scene_ = CreateScope<Scene>("Main Game", SceneSettings{.EnablePhysics = true});

    Application::Get().SetActiveScene(scene_.get());

    // Load map from file
    auto mapResult = se::MapLoader::Load(*scene_, "assets/maps/test.mstmap");
    if (mapResult.success) {
        SE_LOG_INFO("Loaded map: {} entities", mapResult.entityCount);
    }

    character_entity_ = scene_->CreateEntity("Character");

    // IMPORTANT: Set position BEFORE adding physics components!
    // RigidbodyComponent::Awake() reads the TransformComponent position.
    if (mapResult.hasPlayerStart) {
        auto& transform = character_entity_.GetComponent<se::TransformComponent>();
        transform.SetPosition(mapResult.playerStartPosition);
        transform.SetRotation(mapResult.playerStartRotation);
    }

    character_entity_.AddComponent<Character>();
    character_entity_.AddComponent<CameraController>();
    character_entity_.AddComponent<CharacterController>();
    character_entity_.AddComponent<CharacterRender>();
    
    // Set up HDR IBL environment lighting for skybox and reflections
    auto& renderer = Application::Get().GetRenderer().GetSceneRenderer();
    
    se::IBLData ibl;
    ibl.SetDefaultOutdoor();  // SH fallback
    
    // Try to load HDR environment map
    std::filesystem::path hdrPath = "assets/textures/ibl/the_sky_is_on_fire_4k.hdr";
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
            SE_LOG_INFO("[MainGameLayer] Loaded HDR IBL: {}", hdrPath.filename().string());
        } else {
            SE_LOG_WARN("[MainGameLayer] Failed to process HDR, using SH fallback");
        }
    } else {
        SE_LOG_WARN("[MainGameLayer] HDR file not found: {}, using SH fallback", hdrPath.string());
    }
    renderer.SetEnvironmentLighting(ibl);
    
    // Enable Cascaded Shadow Maps for better shadow quality
    renderer.SetCSMEnabled(true);
    renderer.SetCSMSplitLambda(0.85f);

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
        SE_LOG_INFO("[MainGameLayer] Created new directional light entity");
    }
    
    SE_LOG_INFO("[MainGameLayer] Scene initialized with IBL and CSM");
}

void MainGameLayer::OnDetach() {
    Application::Get().SetActiveScene(nullptr);
    Layer::OnDetach();
}

void MainGameLayer::OnUpdate(float ts) {
    Layer::OnUpdate(ts);
    scene_->OnUpdate(ts);
}

void MainGameLayer::OnRender() {
    Layer::OnRender();
    scene_->OnRender();
}

void MainGameLayer::OnImGuiRender() {
    Layer::OnImGuiRender();
    ImguiDebug();
}

} // namespace FirstGame