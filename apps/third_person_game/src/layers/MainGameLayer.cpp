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
#include "engine/renderer/gi/RCDebugRenderer.h"
#include "engine/renderer/gi/RCProfiler.h"


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
    
    // Radiance Cascades Debug Controls (2D Screen-Space)
    ImGui::Separator();
    if (ImGui::CollapsingHeader("Radiance Cascades 2D (Screen-Space)", ImGuiTreeNodeFlags_None)) {
        auto& sceneRenderer = app.GetRenderer().GetSceneRenderer();
        
        bool rcEnabled = sceneRenderer.IsRadianceCascadesEnabled();
        if (ImGui::Checkbox("Enable 2D GI", &rcEnabled)) {
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
                if (ImGui::Combo("Debug Mode##2D", &debugMode, modes, IM_ARRAYSIZE(modes))) {
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
    
    // Sparse Radiance Cascades (3D World-Space GI)
    if (ImGui::CollapsingHeader("Sparse RC (World-Space GI)", ImGuiTreeNodeFlags_DefaultOpen)) {
        auto& sceneRenderer = app.GetRenderer().GetSceneRenderer();
        
        bool sparseRCEnabled = sceneRenderer.IsSparseRCEnabled();
        if (ImGui::Checkbox("Enable World-Space GI", &sparseRCEnabled)) {
            sceneRenderer.SetSparseRCEnabled(sparseRCEnabled);
        }
        
        if (sparseRCEnabled) {
            auto* sparseRC = sceneRenderer.GetSparseRadianceCascades();
            if (sparseRC) {
                ImGui::Text("Sparse RC Status: Active");
                
                auto& config = sparseRC->GetConfig();
                
                // GI Intensity
                ImGui::SliderFloat("GI Intensity", &config.GIIntensity, 0.0f, 5.0f, "%.2f");
                
                // Cascade settings
                if (ImGui::TreeNode("Cascade Settings")) {
                    ImGui::Text("Cascades: %d", config.NumCascades);
                    ImGui::Text("Brick Size: %d³ probes", config.BrickSize);
                    ImGui::SliderFloat("Base Voxel Size", &config.BaseVoxelSize, 0.1f, 2.0f, "%.2f");
                    ImGui::SliderFloat("Cascade Scale", &config.CascadeScale, 1.5f, 4.0f, "%.1f");
                    ImGui::SliderInt("SH Samples/Probe", &config.SHSamplesPerProbe, 16, 128);
                    ImGui::TreePop();
                }
                
                // Debug Visualization
                if (ImGui::TreeNode("Debug Visualization")) {
                    static int debugMode = 0;
                    const char* debugModes[] = { "Off", "Active Bricks", "Radiance Slice", "Irradiance Overlay", "Cascade Levels" };
                    if (ImGui::Combo("Debug Mode##3D", &debugMode, debugModes, IM_ARRAYSIZE(debugModes))) {
                        config.DebugMode = static_cast<se::gi::RCDebugMode>(debugMode);
                    }
                    
                    if (debugMode > 0) {
                        ImGui::Text("Debug visualization active");
                    }
                    ImGui::TreePop();
                }
                
                // Profiling Info
                if (ImGui::TreeNode("Performance")) {
                    auto* profiler = sparseRC->GetProfiler();
                    if (profiler && profiler->IsEnabled()) {
                        const auto& avg = profiler->GetAverageData();
                        ImGui::Text("Total: %.2f ms", avg.totalMs);
                        ImGui::Text("Voxelization: %.2f ms", avg.voxelizationMs);
                        ImGui::Text("Merge: %.2f ms", avg.cascadeMergeMs);
                        ImGui::Text("Resolve: %.2f ms", avg.resolveMs);
                        
                        ImGui::Separator();
                        for (int i = 0; i < 4; i++) {
                            ImGui::Text("Cascade %d: %.2f ms (%d bricks)", i, 
                                        avg.cascadeUpdateMs[i], avg.activeBricks[i]);
                        }
                    }
                    ImGui::TreePop();
                }
                
                // GI Texture Preview
                uint32_t giTex = sparseRC->GetRadianceTexture();
                if (giTex != 0) {
                    ImGui::Text("GI Texture Preview:");
                    ImVec2 previewSize(200, 150);
                    ImGui::Image((ImTextureID)(intptr_t)giTex, previewSize, ImVec2(0, 1), ImVec2(1, 0));
                }
            } else {
                ImGui::Text("Sparse RC: Not initialized");
            }
        } else {
            ImGui::Text("Sparse RC: Disabled");
        }
        
        // Voxelizer Debug
        auto* voxelizer = sceneRenderer.GetSceneVoxelizer();
        if (voxelizer && voxelizer->IsInitialized()) {
            if (ImGui::TreeNode("Voxel Grid Debug")) {
                const auto& voxConfig = voxelizer->GetConfig();
                ImGui::Text("Resolution: %d³", voxConfig.Resolution);
                ImGui::Text("World Size: %.1f", voxConfig.WorldSize);
                ImGui::Text("Center: (%.1f, %.1f, %.1f)", 
                            voxConfig.Center.x, voxConfig.Center.y, voxConfig.Center.z);
                
                ImGui::Text("Albedo Tex ID: %u", voxelizer->GetVoxelTexture());
                ImGui::Text("Emissive Tex ID: %u", voxelizer->GetVoxelEmissiveTexture());
                ImGui::TreePop();
            }
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