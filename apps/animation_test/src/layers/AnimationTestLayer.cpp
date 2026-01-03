#include "AnimationTestLayer.h"

#include "../components/AdvancedCharacterAnimator.h"

#include "engine/Application.h"
#include "engine/Camera.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/ecs/SkinnedModelComponent.h"
#include "engine/ecs/AnimatorComponent.h"
#include "engine/input/InputManager.h"
#include "engine/renderer/IBLProcessor.h"
#include "engine/animation/SkinnedModelManager.h"
#include "engine/debug/DebugRenderer.h"
#include "engine/resources/MapLoader.h"

#include <imgui.h>
#include <filesystem>

namespace AnimationTest {

AnimationTestLayer::~AnimationTestLayer() = default;

void AnimationTestLayer::OnAttach() {
    Layer::OnAttach();
    
    scene_ = se::CreateScope<se::Scene>("Animation Test", se::SceneSettings{.EnablePhysics = true});
    se::Application::Get().SetActiveScene(scene_.get());
    
    SetupScene();
    SetupLighting();
    SetupPlayer();
    
    SE_LOG_INFO("[AnimationTestLayer] Animation test layer initialized");
}

void AnimationTestLayer::OnDetach() {
    se::Application::Get().SetActiveScene(nullptr);
    Layer::OnDetach();
}

void AnimationTestLayer::OnUpdate(float ts) {
    Layer::OnUpdate(ts);
    scene_->OnUpdate(ts);
}

void AnimationTestLayer::OnRender() {
    Layer::OnRender();
    scene_->OnRender();
    
    // Render debug after scene
    Camera* camera = scene_->GetActiveCamera();
    if (camera) {
        se::DebugRenderer::Get().Flush(*camera);
    }
}

void AnimationTestLayer::OnImGuiRender() {
    Layer::OnImGuiRender();
    RenderDebugUI();
}

void AnimationTestLayer::SetupScene() {
    // Scene setup is minimal - map is loaded in SetupPlayer
    // IBL and lighting are handled in SetupLighting
}

void AnimationTestLayer::SetupLighting() {
    // IBL Environment
    auto& renderer = se::Application::Get().GetRenderer().GetSceneRenderer();
    
    se::IBLData ibl;
    ibl.SetDefaultOutdoor();
    
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
            SE_LOG_INFO("[AnimationTestLayer] Loaded HDR IBL");
        }
    }
    renderer.SetEnvironmentLighting(ibl);
    renderer.SetCSMEnabled(true);
    
    // Directional light
    lightEntity_ = scene_->CreateEntity("Sun");
    auto& lightTransform = lightEntity_.GetComponent<se::TransformComponent>();
    lightTransform.SetPosition({10.0f, 20.0f, 10.0f});
    lightTransform.SetRotation({45.0f, 45.0f, 0.0f});
    
    auto& light = lightEntity_.AddComponent<se::DirectionalLightComponent>();
    light.Color = {1.0f, 0.95f, 0.9f};
    light.Intensity = 2.0f;
    light.CastShadows = true;
    light.Enabled = true;
}

void AnimationTestLayer::SetupPlayer() {
    playerEntity_ = scene_->CreateEntity("Player");

    // Load map may have set a start position
    auto mapResult = se::MapLoader::Load(*scene_, "assets/maps/test.mstmap");
    auto& transform = playerEntity_.GetComponent<se::TransformComponent>();
    if (mapResult.hasPlayerStart) {
        transform.SetPosition(mapResult.playerStartPosition);
        transform.SetRotation(mapResult.playerStartRotation);
    } else {
        transform.SetPosition({0.0f, 1.0f, 0.0f});
    }

    // Add Character (Pawn with physics movement)
    playerEntity_.AddComponent<se::Character>();
    
    // Add PlayerController (handles input and camera)
    playerEntity_.AddComponent<se::PlayerController>();
    
    // Add visual representation with advanced animations
    playerEntity_.AddComponent<AdvancedCharacterAnimator>();
    
    SE_LOG_INFO("[AnimationTestLayer] Player setup complete with AdvancedCharacterAnimator");
}

void AnimationTestLayer::RenderDebugUI() {
    ImGui::Begin("Animation Test Debug");
    
    ImGui::Text("FPS: %.0f", ImGui::GetIO().Framerate);
    ImGui::Separator();
    
    ImGui::Checkbox("Show Skeleton Debug", &showSkeletonDebug_);
    ImGui::Checkbox("Show Blend Space Debug", &showBlendSpaceDebug_);
    ImGui::Checkbox("Show Layer Debug", &showLayerDebug_);
    ImGui::Checkbox("Show Look At Debug", &showLookAtDebug_);
    
    ImGui::Separator();
    ImGui::Text("Controls:");
    ImGui::BulletText("WASD - Move");
    ImGui::BulletText("Shift - Sprint");
    ImGui::BulletText("RMB - Aim Mode (toggle strafe)");
    ImGui::BulletText("Mouse - Look Direction");
    
    ImGui::End();
    
    if (showBlendSpaceDebug_) {
        RenderBlendSpaceDebugPanel();
    }
    
    if (showLayerDebug_) {
        RenderLayerDebugPanel();
    }
    
    RenderAnimationDebugPanel();
}

void AnimationTestLayer::RenderAnimationDebugPanel() {
    if (!playerEntity_.IsValid()) return;
    
    auto* animator = playerEntity_.FindComponent<AdvancedCharacterAnimator>();
    if (!animator) return;
    
    ImGui::Begin("Character Animation State");
    
    // Mode display
    const char* modeNames[] = {"Standing", "Aiming"};
    ImGui::Text("Mode: %s", modeNames[static_cast<int>(animator->GetLocomotionMode())]);
    
    // Velocity
    ImGui::Text("Velocity: %.2f m/s", animator->GetCurrentVelocity());
    
    // Strafe input (during aim mode)
    auto strafeInput = animator->GetStrafeInput();
    ImGui::Text("Strafe Input: (%.2f, %.2f)", strafeInput.x, strafeInput.y);
    
    ImGui::Separator();
    
    // Look At state
    if (showLookAtDebug_) {
        auto lookAtAngles = animator->GetLookAtAngles();
        ImGui::Text("Look At: H=%.1f° V=%.1f°", lookAtAngles.x, lookAtAngles.y);
    }
    
    // Body rotation
    ImGui::Text("Body Yaw: %.1f°", animator->GetBodyYaw());
    ImGui::Text("Body Rotating: %s", animator->IsBodyRotating() ? "Yes" : "No");
    
    ImGui::End();
}

void AnimationTestLayer::RenderBlendSpaceDebugPanel() {
    ImGui::Begin("Blend Space Debug");
    
    auto* animator = playerEntity_.FindComponent<AdvancedCharacterAnimator>();
    if (!animator) {
        ImGui::Text("No animator found");
        ImGui::End();
        return;
    }
    
    // Draw 2D blend space visualization
    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize(200.0f, 200.0f);
    
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    // Background
    drawList->AddRectFilled(canvasPos, 
                            ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y),
                            IM_COL32(40, 40, 40, 255));
    
    // Grid lines
    ImVec2 center(canvasPos.x + canvasSize.x * 0.5f, canvasPos.y + canvasSize.y * 0.5f);
    drawList->AddLine(ImVec2(center.x, canvasPos.y), 
                      ImVec2(center.x, canvasPos.y + canvasSize.y),
                      IM_COL32(80, 80, 80, 255));
    drawList->AddLine(ImVec2(canvasPos.x, center.y), 
                      ImVec2(canvasPos.x + canvasSize.x, center.y),
                      IM_COL32(80, 80, 80, 255));
    
    // Sample points (corners and center)
    float sampleRadius = 6.0f;
    ImU32 sampleColor = IM_COL32(100, 150, 255, 255);
    
    // Forward
    drawList->AddCircleFilled(ImVec2(center.x, canvasPos.y + 20.0f), sampleRadius, sampleColor);
    // Backward
    drawList->AddCircleFilled(ImVec2(center.x, canvasPos.y + canvasSize.y - 20.0f), sampleRadius, sampleColor);
    // Left
    drawList->AddCircleFilled(ImVec2(canvasPos.x + 20.0f, center.y), sampleRadius, sampleColor);
    // Right
    drawList->AddCircleFilled(ImVec2(canvasPos.x + canvasSize.x - 20.0f, center.y), sampleRadius, sampleColor);
    // Center
    drawList->AddCircleFilled(center, sampleRadius, sampleColor);
    
    // Current position (based on strafe input)
    auto strafeInput = animator->GetStrafeInput();
    float posX = center.x + strafeInput.x * (canvasSize.x * 0.4f);
    float posY = center.y - strafeInput.y * (canvasSize.y * 0.4f);  // Y inverted
    
    drawList->AddCircleFilled(ImVec2(posX, posY), 8.0f, IM_COL32(255, 100, 100, 255));
    
    ImGui::Dummy(canvasSize);
    
    ImGui::Text("X: %.2f, Y: %.2f", strafeInput.x, strafeInput.y);
    
    ImGui::End();
}

void AnimationTestLayer::RenderLayerDebugPanel() {
    ImGui::Begin("Animation Layers");
    
    auto* animator = playerEntity_.FindComponent<AdvancedCharacterAnimator>();
    if (!animator) {
        ImGui::Text("No animator found");
        ImGui::End();
        return;
    }
    
    // Display layer weights
    ImGui::Text("Layer Stack:");
    ImGui::Separator();
    
    // Base Locomotion
    float baseWeight = 1.0f;
    ImGui::ProgressBar(baseWeight, ImVec2(-1, 0), "Base Locomotion");
    
    // Upper Body Aim
    float aimWeight = animator->GetAimLayerWeight();
    ImGui::ProgressBar(aimWeight, ImVec2(-1, 0), "Upper Body Aim");
    
    // Look At
    float lookAtWeight = animator->IsLookAtEnabled() ? 1.0f : 0.0f;
    ImGui::ProgressBar(lookAtWeight, ImVec2(-1, 0), "Look At (Additive)");
    
    ImGui::End();
}

} // namespace AnimationTest
