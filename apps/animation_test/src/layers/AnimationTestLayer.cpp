#include "AnimationTestLayer.h"

#include "../components/AdvancedCharacterAnimator.h"

#include "engine/Application.h"
#include "engine/Camera.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/ecs/SkinnedModelComponent.h"
#include "engine/ecs/AnimatorComponent.h"
#include "engine/input/InputManager.h"
#include "engine/input/GamepadManager.h"
#include "engine/input/GamepadCodes.h"
#include "engine/renderer/IBLProcessor.h"
#include "engine/animation/SkinnedModelManager.h"
#include "engine/debug/DebugRenderer.h"
#include "engine/resources/MapLoader.h"
#include "engine/ui/native/world/WorldSpaceUIRenderer.h"
#include "engine/ui/native/UISystem.h"

#include <imgui.h>
#include <filesystem>
#include <cmath>

namespace AnimationTest {

AnimationTestLayer::~AnimationTestLayer() = default;

void AnimationTestLayer::OnAttach() {
    Layer::OnAttach();
    
    scene_ = se::CreateScope<se::Scene>("Animation Test", se::SceneSettings{.EnablePhysics = true});
    se::Application::Get().SetActiveScene(scene_.get());
    
    // Initialize UI system for WorldSpaceUI text rendering
    auto& window = se::Application::Get().GetWindow();
    float viewportW = static_cast<float>(window.GetWidth());
    float viewportH = static_cast<float>(window.GetHeight());
    se::ui::Initialize(viewportW, viewportH);
    
    SetupScene();
    SetupLighting();
    SetupPlayer();
    
    SE_LOG_INFO("[AnimationTestLayer] Animation test layer initialized");
}

void AnimationTestLayer::OnDetach() {
    se::ui::Shutdown();
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
        
        // Render WorldSpace UI (bone labels, etc.)
        se::WorldSpaceUIRenderer::Get().Render(*camera, scene_->GetRegistry());
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
    ImGui::Text("Keyboard/Mouse Controls:");
    ImGui::BulletText("WASD - Move");
    ImGui::BulletText("Shift - Sprint");
    ImGui::BulletText("RMB - Aim Mode (toggle strafe)");
    ImGui::BulletText("Mouse - Look Direction");
    
    ImGui::Separator();
    ImGui::Text("Gamepad Controls:");
    ImGui::BulletText("Left Stick - Move");
    ImGui::BulletText("Right Stick - Look");
    ImGui::BulletText("A - Jump");
    ImGui::BulletText("Start - Toggle Mouse");
    
    ImGui::End();
    
    // Gamepad Debug Panel
    RenderGamepadDebugPanel();
    
    if (showBlendSpaceDebug_) {
        RenderBlendSpaceDebugPanel();
    }
    
    if (showLayerDebug_) {
        RenderLayerDebugPanel();
    }
    
    RenderAnimationDebugPanel();
}

void AnimationTestLayer::RenderGamepadDebugPanel() {
    auto& gamepadMgr = se::GamepadManager::Get();
    
    ImGui::Begin("Gamepad Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    
    int connectedCount = gamepadMgr.GetConnectedCount();
    if (connectedCount == 0) {
        ImGui::TextColored(ImVec4(1.0f, 0.4f, 0.4f, 1.0f), "No gamepad connected");
        ImGui::Text("Connect a controller to see input");
        ImGui::End();
        return;
    }
    
    ImGui::TextColored(ImVec4(0.4f, 1.0f, 0.4f, 1.0f), "Gamepad Connected: %d", connectedCount);
    
    se::GamepadId id = gamepadMgr.GetFirstConnectedId();
    const char* name = gamepadMgr.GetName(id);
    ImGui::Text("Name: %s", name ? name : "Unknown");
    ImGui::Separator();
    
    const auto& state = gamepadMgr.GetState(id);
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    
    // Colors
    ImU32 bgColor = IM_COL32(40, 40, 50, 255);
    ImU32 borderColor = IM_COL32(100, 100, 120, 255);
    ImU32 stickBgColor = IM_COL32(30, 30, 40, 255);
    ImU32 stickColor = IM_COL32(100, 150, 255, 255);
    ImU32 stickActiveColor = IM_COL32(150, 200, 255, 255);
    ImU32 buttonOffColor = IM_COL32(60, 60, 80, 255);
    ImU32 buttonOnColor = IM_COL32(100, 200, 100, 255);
    ImU32 triggerBgColor = IM_COL32(50, 50, 70, 255);
    ImU32 triggerFillColor = IM_COL32(255, 150, 50, 255);
    
    ImVec2 startPos = ImGui::GetCursorScreenPos();
    float panelWidth = 400.0f;
    float panelHeight = 250.0f;
    
    // Background
    drawList->AddRectFilled(startPos, ImVec2(startPos.x + panelWidth, startPos.y + panelHeight), bgColor, 10.0f);
    drawList->AddRect(startPos, ImVec2(startPos.x + panelWidth, startPos.y + panelHeight), borderColor, 10.0f);
    
    // --- Left Stick ---
    float leftStickX = startPos.x + 70.0f;
    float leftStickY = startPos.y + 100.0f;
    float stickRadius = 40.0f;
    float stickDotRadius = 12.0f;
    
    drawList->AddCircleFilled(ImVec2(leftStickX, leftStickY), stickRadius, stickBgColor);
    drawList->AddCircle(ImVec2(leftStickX, leftStickY), stickRadius, borderColor, 32, 2.0f);
    
    float lx = gamepadMgr.GetAxis(id, se::Gamepad::LeftX);
    float ly = gamepadMgr.GetAxis(id, se::Gamepad::LeftY);
    float dotX = leftStickX + lx * (stickRadius - stickDotRadius);
    float dotY = leftStickY + ly * (stickRadius - stickDotRadius);
    bool leftActive = std::abs(lx) > 0.01f || std::abs(ly) > 0.01f;
    drawList->AddCircleFilled(ImVec2(dotX, dotY), stickDotRadius, leftActive ? stickActiveColor : stickColor);
    
    // Left stick label
    drawList->AddText(ImVec2(leftStickX - 10.0f, leftStickY + stickRadius + 5.0f), IM_COL32(200, 200, 200, 255), "L");
    
    // L3 button (press left stick)
    bool l3 = state.buttons[se::Gamepad::LeftThumb];
    drawList->AddText(ImVec2(leftStickX - 8.0f, leftStickY - 6.0f), l3 ? buttonOnColor : IM_COL32(100, 100, 100, 255), "L3");
    
    // --- Right Stick ---
    float rightStickX = startPos.x + 330.0f;
    float rightStickY = startPos.y + 160.0f;
    
    drawList->AddCircleFilled(ImVec2(rightStickX, rightStickY), stickRadius, stickBgColor);
    drawList->AddCircle(ImVec2(rightStickX, rightStickY), stickRadius, borderColor, 32, 2.0f);
    
    float rx = gamepadMgr.GetAxis(id, se::Gamepad::RightX);
    float ry = gamepadMgr.GetAxis(id, se::Gamepad::RightY);
    dotX = rightStickX + rx * (stickRadius - stickDotRadius);
    dotY = rightStickY + ry * (stickRadius - stickDotRadius);
    bool rightActive = std::abs(rx) > 0.01f || std::abs(ry) > 0.01f;
    drawList->AddCircleFilled(ImVec2(dotX, dotY), stickDotRadius, rightActive ? stickActiveColor : stickColor);
    
    // Right stick label
    drawList->AddText(ImVec2(rightStickX - 10.0f, rightStickY + stickRadius + 5.0f), IM_COL32(200, 200, 200, 255), "R");
    
    // R3 button
    bool r3 = state.buttons[se::Gamepad::RightThumb];
    drawList->AddText(ImVec2(rightStickX - 8.0f, rightStickY - 6.0f), r3 ? buttonOnColor : IM_COL32(100, 100, 100, 255), "R3");
    
    // --- D-Pad ---
    float dpadX = startPos.x + 70.0f;
    float dpadY = startPos.y + 190.0f;
    float dpadSize = 18.0f;
    float dpadGap = 2.0f;
    
    // Up
    bool dUp = state.buttons[se::Gamepad::DPadUp];
    drawList->AddRectFilled(
        ImVec2(dpadX - dpadSize/2, dpadY - dpadSize*1.5f - dpadGap),
        ImVec2(dpadX + dpadSize/2, dpadY - dpadSize/2 - dpadGap),
        dUp ? buttonOnColor : buttonOffColor);
    
    // Down
    bool dDown = state.buttons[se::Gamepad::DPadDown];
    drawList->AddRectFilled(
        ImVec2(dpadX - dpadSize/2, dpadY + dpadSize/2 + dpadGap),
        ImVec2(dpadX + dpadSize/2, dpadY + dpadSize*1.5f + dpadGap),
        dDown ? buttonOnColor : buttonOffColor);
    
    // Left
    bool dLeft = state.buttons[se::Gamepad::DPadLeft];
    drawList->AddRectFilled(
        ImVec2(dpadX - dpadSize*1.5f - dpadGap, dpadY - dpadSize/2),
        ImVec2(dpadX - dpadSize/2 - dpadGap, dpadY + dpadSize/2),
        dLeft ? buttonOnColor : buttonOffColor);
    
    // Right
    bool dRight = state.buttons[se::Gamepad::DPadRight];
    drawList->AddRectFilled(
        ImVec2(dpadX + dpadSize/2 + dpadGap, dpadY - dpadSize/2),
        ImVec2(dpadX + dpadSize*1.5f + dpadGap, dpadY + dpadSize/2),
        dRight ? buttonOnColor : buttonOffColor);
    
    // Center
    drawList->AddRectFilled(
        ImVec2(dpadX - dpadSize/2, dpadY - dpadSize/2),
        ImVec2(dpadX + dpadSize/2, dpadY + dpadSize/2),
        buttonOffColor);
    
    // --- Face Buttons (A, B, X, Y) ---
    float faceX = startPos.x + 330.0f;
    float faceY = startPos.y + 70.0f;
    float faceRadius = 14.0f;
    float faceSpacing = 25.0f;
    
    // A (bottom)
    bool btnA = state.buttons[se::Gamepad::A];
    drawList->AddCircleFilled(ImVec2(faceX, faceY + faceSpacing), faceRadius, btnA ? IM_COL32(100, 255, 100, 255) : buttonOffColor);
    drawList->AddText(ImVec2(faceX - 4, faceY + faceSpacing - 6), IM_COL32(255, 255, 255, 255), "A");
    
    // B (right)
    bool btnB = state.buttons[se::Gamepad::B];
    drawList->AddCircleFilled(ImVec2(faceX + faceSpacing, faceY), faceRadius, btnB ? IM_COL32(255, 100, 100, 255) : buttonOffColor);
    drawList->AddText(ImVec2(faceX + faceSpacing - 4, faceY - 6), IM_COL32(255, 255, 255, 255), "B");
    
    // X (left)
    bool btnX = state.buttons[se::Gamepad::X];
    drawList->AddCircleFilled(ImVec2(faceX - faceSpacing, faceY), faceRadius, btnX ? IM_COL32(100, 100, 255, 255) : buttonOffColor);
    drawList->AddText(ImVec2(faceX - faceSpacing - 4, faceY - 6), IM_COL32(255, 255, 255, 255), "X");
    
    // Y (top)
    bool btnY = state.buttons[se::Gamepad::Y];
    drawList->AddCircleFilled(ImVec2(faceX, faceY - faceSpacing), faceRadius, btnY ? IM_COL32(255, 255, 100, 255) : buttonOffColor);
    drawList->AddText(ImVec2(faceX - 4, faceY - faceSpacing - 6), IM_COL32(255, 255, 255, 255), "Y");
    
    // --- Bumpers (LB, RB) ---
    float bumperY = startPos.y + 15.0f;
    float bumperW = 50.0f;
    float bumperH = 18.0f;
    
    // LB
    bool lb = state.buttons[se::Gamepad::LeftBumper];
    drawList->AddRectFilled(
        ImVec2(startPos.x + 40.0f, bumperY),
        ImVec2(startPos.x + 40.0f + bumperW, bumperY + bumperH),
        lb ? buttonOnColor : buttonOffColor, 4.0f);
    drawList->AddText(ImVec2(startPos.x + 55.0f, bumperY + 2.0f), IM_COL32(255, 255, 255, 255), "LB");
    
    // RB
    bool rb = state.buttons[se::Gamepad::RightBumper];
    drawList->AddRectFilled(
        ImVec2(startPos.x + panelWidth - 40.0f - bumperW, bumperY),
        ImVec2(startPos.x + panelWidth - 40.0f, bumperY + bumperH),
        rb ? buttonOnColor : buttonOffColor, 4.0f);
    drawList->AddText(ImVec2(startPos.x + panelWidth - 40.0f - bumperW + 15.0f, bumperY + 2.0f), IM_COL32(255, 255, 255, 255), "RB");
    
    // --- Triggers (LT, RT) ---
    float triggerY = startPos.y + 40.0f;
    float triggerW = 50.0f;
    float triggerH = 35.0f;
    
    // LT
    float lt = (gamepadMgr.GetAxisRaw(id, se::Gamepad::LeftTrigger) + 1.0f) * 0.5f; // Normalize to 0-1
    drawList->AddRectFilled(
        ImVec2(startPos.x + 40.0f, triggerY),
        ImVec2(startPos.x + 40.0f + triggerW, triggerY + triggerH),
        triggerBgColor, 4.0f);
    drawList->AddRectFilled(
        ImVec2(startPos.x + 40.0f, triggerY + triggerH * (1.0f - lt)),
        ImVec2(startPos.x + 40.0f + triggerW, triggerY + triggerH),
        triggerFillColor, 4.0f);
    drawList->AddText(ImVec2(startPos.x + 55.0f, triggerY + 10.0f), IM_COL32(255, 255, 255, 255), "LT");
    
    // RT
    float rt = (gamepadMgr.GetAxisRaw(id, se::Gamepad::RightTrigger) + 1.0f) * 0.5f;
    drawList->AddRectFilled(
        ImVec2(startPos.x + panelWidth - 40.0f - triggerW, triggerY),
        ImVec2(startPos.x + panelWidth - 40.0f, triggerY + triggerH),
        triggerBgColor, 4.0f);
    drawList->AddRectFilled(
        ImVec2(startPos.x + panelWidth - 40.0f - triggerW, triggerY + triggerH * (1.0f - rt)),
        ImVec2(startPos.x + panelWidth - 40.0f, triggerY + triggerH),
        triggerFillColor, 4.0f);
    drawList->AddText(ImVec2(startPos.x + panelWidth - 40.0f - triggerW + 15.0f, triggerY + 10.0f), IM_COL32(255, 255, 255, 255), "RT");
    
    // --- Start / Back ---
    float centerX = startPos.x + panelWidth / 2.0f;
    float centerY = startPos.y + 80.0f;
    float smallBtnW = 35.0f;
    float smallBtnH = 14.0f;
    
    // Back/Select
    bool back = state.buttons[se::Gamepad::Back];
    drawList->AddRectFilled(
        ImVec2(centerX - 50.0f, centerY),
        ImVec2(centerX - 50.0f + smallBtnW, centerY + smallBtnH),
        back ? buttonOnColor : buttonOffColor, 3.0f);
    drawList->AddText(ImVec2(centerX - 45.0f, centerY), IM_COL32(200, 200, 200, 255), "SEL");
    
    // Start
    bool start = state.buttons[se::Gamepad::Start];
    drawList->AddRectFilled(
        ImVec2(centerX + 15.0f, centerY),
        ImVec2(centerX + 15.0f + smallBtnW, centerY + smallBtnH),
        start ? buttonOnColor : buttonOffColor, 3.0f);
    drawList->AddText(ImVec2(centerX + 17.0f, centerY), IM_COL32(200, 200, 200, 255), "STA");
    
    // Guide button
    bool guide = state.buttons[se::Gamepad::Guide];
    drawList->AddCircleFilled(ImVec2(centerX, centerY + 30.0f), 12.0f, guide ? buttonOnColor : buttonOffColor);
    
    // Reserve space
    ImGui::Dummy(ImVec2(panelWidth, panelHeight));
    
    // Axis values text
    ImGui::Separator();
    ImGui::Text("Left Stick:  X: %+.2f  Y: %+.2f", lx, ly);
    ImGui::Text("Right Stick: X: %+.2f  Y: %+.2f", rx, ry);
    ImGui::Text("Triggers:    LT: %.2f  RT: %.2f", lt, rt);
    
    ImGui::End();
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
    
    ImGui::Separator();
    
    // Aim offset debug (with blendspace visualization)
    animator->RenderImGuiDebug();
    
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
