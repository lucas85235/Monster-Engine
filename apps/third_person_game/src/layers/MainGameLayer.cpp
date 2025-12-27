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

namespace FirstGame {

void ImguiDebug() {
    auto& app    = se::Application::Get();
    auto& window = app.GetWindow();

    ImGui::Begin("Main Game Debug");
    ImGui::Text("Debug Info");
    ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
    ImGui::Text("Current Resolution: [%d x %d]", window.GetWidth(), window.GetHeight());
    ImGui::Separator();
    ImGui::Text("Press TAB to toggle mouse capture");
    
    // Radiance Cascades Debug Controls
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Radiance Cascades (GI)", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& sceneRenderer = app.GetRenderer().GetSceneRenderer();
        
        bool rcEnabled = sceneRenderer.IsRadianceCascadesEnabled();
        if (ImGui::Checkbox("Enable GI", &rcEnabled)) {
            sceneRenderer.SetRadianceCascadesEnabled(rcEnabled);
        }
        
        if (rcEnabled) {
            auto* rcPass = sceneRenderer.GetRadianceCascadesPass();
            if (rcPass) {
                ImGui::Text("RC Status: Active");
                ImGui::Text("Radiance Texture ID: %u", rcPass->GetRadianceTexture());
                
                // Debug visualization mode
                static int debugMode = 0;
                const char* modes[] = { "Off", "Show GI Only", "Apply to Scene" };
                if (ImGui::Combo("Debug Mode", &debugMode, modes, IM_ARRAYSIZE(modes))) {
                    // Mode will be used below
                }
                
                // Show miniature of GI texture if available
                uint32_t giTex = rcPass->GetRadianceTexture();
                if (giTex != 0 && debugMode > 0) {
                    ImGui::Text("GI Texture Preview:");
                    ImVec2 previewSize(200, 150);
                    ImGui::Image((ImTextureID)(intptr_t)giTex, previewSize, ImVec2(0, 1), ImVec2(1, 0));
                }
            } else {
                ImGui::Text("RC Status: Not initialized");
            }
        } else {
            ImGui::Text("RC Status: Disabled");
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