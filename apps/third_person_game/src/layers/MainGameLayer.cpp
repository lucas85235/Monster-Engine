#include "MainGameLayer.h"

#include "../../../SampleUtilities.h"
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

// Native UI System
#include "engine/ui/native/widgets/hud/UICrosshair.h"
#include "engine/ui/native/widgets/hud/UIHealthBar.h"
#include "engine/ui/native/widgets/hud/UIAbilitySlot.h"
#include "engine/ui/native/world/WorldSpaceUIComponent.h"
#include "engine/ui/native/world/WorldSpaceUIRenderer.h"
#include "engine/debug/FrameProfiler.h"
#include "engine/debug/DebugRenderer.h"

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
    
    // Post-Processing Controls Window
    ImGui::Begin("Post-Processing");
    
    auto& renderer = se::Application::Get().GetRenderer().GetSceneRenderer();
    
    // Master enable/disable
    bool postProcessEnabled = renderer.IsPostProcessEnabled();
    if (ImGui::Checkbox("Enable Post-Processing", &postProcessEnabled)) {
        renderer.SetPostProcessEnabled(postProcessEnabled);
    }
    
    // Individual pass controls
    if (postProcessEnabled) {
        auto* pipeline = renderer.GetPostProcessPipeline();
        if (pipeline) {
            ImGui::Separator();
            pipeline->RenderUI();
        }
    }
    
    ImGui::End();
    
    // AI Debug Window - MANUAL CONTROL MODE
    ImGui::Begin("AI Debug");
    ImGui::TextColored(ImVec4(1,1,0,1), "DEBUG MODE - Manual Control Only");
    ImGui::Separator();
    
    if (enemyEntity_.IsValid()) {
        auto* aiController = enemyEntity_.FindComponent<se::AIController>();
        if (aiController) {
            auto& agent = aiController->GetPathfindingAgent();
            
            // Show agent state
            const char* stateNames[] = {"Idle", "Moving", "Arrived", "Stuck", "RequestingPath"};
            ImGui::Text("State: %s", stateNames[static_cast<int>(agent.state)]);
            ImGui::Text("Has Path: %s", agent.HasPath() ? "Yes" : "No");
            if (agent.HasPath()) {
                ImGui::Text("Waypoints: %d / %zu", agent.currentWaypoint, agent.currentPath.size());
            }
            ImGui::Text("Path Age: %.1fs", agent.pathAge);
            
            ImGui::Separator();
            
            // Force recalculate button
            if (ImGui::Button("Force Recalculate Path")) {
                agent.forceRepath = true;
                SE_LOG_INFO("[AI Debug] Force recalculate requested");
            }
            
            ImGui::SameLine();
            if (ImGui::Button("Stop Movement")) {
                aiController->StopMovement();
            }
            
            ImGui::Separator();
            
            // Rebake navmesh button
            if (navSystem_ && ImGui::Button("Rebake Navmesh")) {
                navSystem_->RebakeObstacles();
                SE_LOG_INFO("[AI Debug] Navmesh rebaked");
            }
            
            // Show navmesh stats
            if (navSystem_ && navSystem_->GetGrid()) {
                auto* grid = navSystem_->GetGrid();
                int walkable = 0, total = 0;
                for (int z = 0; z < grid->GetHeight(); ++z) {
                    for (int x = 0; x < grid->GetWidth(); ++x) {
                        total++;
                        if (grid->IsWalkable(x, z)) walkable++;
                    }
                }
                ImGui::Text("Navmesh: %d walkable / %d total", walkable, total);
                ImGui::Text("Grid: %d x %d", grid->GetWidth(), grid->GetHeight());
            }
        }
    }
    ImGui::End();

    // Navigation Debug - Full Panel
    if (navSystem_) {
        glm::vec3 enemyPos{0.0f};
        auto& navDebug = navSystem_->GetDebug();
        
        if (enemyEntity_.IsValid() && enemyEntity_.HasComponent<se::TransformComponent>()) {
            enemyPos = enemyEntity_.GetComponent<se::TransformComponent>().Position;

            
            // Update path visualization from agent's current path
            auto* aiController = enemyEntity_.FindComponent<se::AIController>();
            if (aiController) {
                auto& agent = aiController->GetPathfindingAgent();
                if (agent.HasPath()) {
                    // Only show waypoints from current index onwards
                    std::vector<glm::vec3> remainingPath(
                        agent.currentPath.begin() + agent.currentWaypoint,
                        agent.currentPath.end()
                    );
                    navDebug.AddActivePath(1, remainingPath);
                } else {
                    navDebug.RemoveActivePath(1);
                }
            }
        }
        
        // Set callback for move-to button
        navDebug.SetMoveToCallback([this](const glm::vec3& target) {
            if (enemyEntity_.IsValid()) {
                auto* aiController = enemyEntity_.FindComponent<se::AIController>();
                if (aiController) {
                    if (std::isnan(target.x)) {
                        aiController->StopMovement();
                    } else {
                        aiController->MoveToLocation(target);
                    }
                }
            }
        });
        
        Camera* camera = scene_->GetActiveCamera();
        if (camera) {
            navDebug.RenderImGuiPanel(navSystem_.get(), enemyPos, *camera);
        }
    }
    
    // HUD Demo Controls
    ImGui::Begin("HUD Controls");
    
    if (hudController_) {
        ImGui::Text("HUD Demo");
        ImGui::Separator();
        
        // Health control
        if (ImGui::SliderFloat("Health", &demoHealth_, 0.0f, 100.0f)) {
            hudController_->SetHealth(demoHealth_);
        }
        
        ImGui::Checkbox("Animate Health", &animateHealth_);
        
        ImGui::Separator();
        
        // Crosshair controls
        if (auto* crosshair = hudController_->GetCrosshair()) {
            ImGui::Text("Crosshair");
            
            static float crosshairLength = 10.0f;
            static float crosshairThickness = 2.0f;
            static float crosshairGap = 3.0f;
            static glm::vec4 crosshairColor = {1.0f, 1.0f, 1.0f, 0.9f};
            
            bool crosshairChanged = false;
            crosshairChanged |= ImGui::SliderFloat("Line Length", &crosshairLength, 4.0f, 30.0f);
            crosshairChanged |= ImGui::SliderFloat("Thickness", &crosshairThickness, 1.0f, 6.0f);
            crosshairChanged |= ImGui::SliderFloat("Gap", &crosshairGap, 0.0f, 15.0f);
            crosshairChanged |= ImGui::ColorEdit4("Color##Crosshair", &crosshairColor.x);
            
            if (crosshairChanged) {
                hudController_->SetCrosshairSize(crosshairLength, crosshairThickness, crosshairGap);
                hudController_->SetCrosshairColor(crosshairColor);
            }
        }
        
        ImGui::Separator();
        
        // Ability slot controls
        ImGui::Text("Ability Slots");
        for (size_t i = 0; i < 3; ++i) {
            if (auto* slot = hudController_->GetAbilitySlot(i)) {
                std::string label = "Cooldown " + std::to_string(i + 1);
                static float cooldowns[3] = {0.0f, 0.0f, 0.0f};
                if (ImGui::SliderFloat(label.c_str(), &cooldowns[i], 0.0f, 1.0f)) {
                    hudController_->SetAbilityCooldown(i, cooldowns[i]);
                }
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

    // Setup navigation first (needed by AI)
    SetupNavigation();
    
    // Setup player using new architecture
    SetupPlayer();
    
    // Setup enemy with AI
    SetupEnemy();
    
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
    
    // Initialize HUD
    SetupHUD();
}

void MainGameLayer::SetupNavigation() {
    navSystem_ = std::make_unique<se::nav::NavigationSystem>(scene_.get());
    navSystem_->Initialize();
    
    // Create navigation grid - covers -100 to +100 on X and Z
    // Using 0.5f cell size for higher resolution pathfinding
    se::nav::NavigationGridSettings gridSettings;
    gridSettings.worldOrigin = {-100.0f, 0.0f, -100.0f};
    gridSettings.width = 400;       // 200 / 0.5 = 400 cells
    gridSettings.height = 400;
    gridSettings.cellSize = 0.5f;   // Smaller cells for better path quality
    gridSettings.agentRadius = 0.8f; // Larger radius for better corner avoidance
    gridSettings.agentHeight = 1.8f;
    gridSettings.allowDiagonal = true;
    
    navSystem_->CreateGrid(gridSettings);
    
    // Bake static obstacles from physics (now cached - only runs once)
    navSystem_->BakeObstacles();
    
    SE_LOG_INFO("[MainGameLayer] Navigation grid: 400x400 (cell 0.5f), origin (-100, -100), agent radius {:.1f}", gridSettings.agentRadius);
}


void MainGameLayer::SetupPlayer() {
    playerEntity_ = scene_->CreateEntity("Player");

    // Load map may have set a start position
    auto mapResult = se::MapLoader::Load(*scene_, "assets/maps/test.mstmap");
    if (mapResult.hasPlayerStart) {
        auto& transform = playerEntity_.GetComponent<se::TransformComponent>();
        transform.SetPosition(mapResult.playerStartPosition);
        transform.SetRotation(mapResult.playerStartRotation);
    }

    // Add Character (Pawn with physics movement)
    playerEntity_.AddComponent<se::Character>();
    
    // Add PlayerController (handles input and camera)
    auto& controller = playerEntity_.AddComponent<se::PlayerController>();
    
    // Add visual representation with animations
    playerEntity_.AddComponent<CharacterRender>();
    
    SE_LOG_INFO("[MainGameLayer] Player setup complete with new Gameplay architecture");
}

void MainGameLayer::SetupEnemy() {
    enemyEntity_ = scene_->CreateEntity("Enemy");
    
    // Position enemy near player
    auto& transform = enemyEntity_.GetComponent<se::TransformComponent>();
    transform.SetPosition({10.0f, 10.0f, 10.0f});
    
    // Add Character for physics movement
    auto& character = enemyEntity_.AddComponent<se::Character>();
    character.GetMovementConfig().maxWalkSpeed = 3.0f;  // Slower than player
    
    // Add AIController
    auto& aiController = enemyEntity_.AddComponent<se::AIController>();
    aiController.SetNavigationSystem(navSystem_.get());
    
    // SIMPLE DEBUG MODE: No state tree, only manual control via ImGui
    // Disable auto-repath - will only calculate path once when MoveTo is called
    auto& agent = aiController.GetPathfindingAgent();
    agent.autoRepath = false;  // Disable automatic recalculation
    
    // Add visual representation with animations (same as player)
    CharacterRenderConfig renderConfig;
    renderConfig.ModelPath = "assets/models/characters/Y_Bot.fbx";
    auto& render = enemyEntity_.AddComponent<CharacterRender>();
    render.SetConfig(renderConfig);
    
    // Register enemy as navigation agent (excluded from obstacle detection)
    navSystem_->RegisterAgent(enemyEntity_);
    
    // Add World Space UI - enemy name above head
    auto& worldUI = enemyEntity_.AddComponent<se::WorldSpaceUIComponent>();
    worldUI.offset = {0.0f, 2.5f, 0.0f};  // Above character head
    worldUI.baseScale = 1.0f;
    worldUI.scaleByDistance = true;
    
    auto nameLabel = worldUI.AddElement<se::WorldSpaceText>();
    nameLabel->text = "Enemy";
    nameLabel->color = {1.0f, 0.2f, 0.2f, 1.0f};  // Red
    nameLabel->fontSize = 18.0f;
    nameLabel->centered = true;
    
    SE_LOG_INFO("[MainGameLayer] Enemy AI setup with World Space UI name label");
}


void MainGameLayer::OnDetach() {
    hudController_.reset();
    navSystem_.reset();
    se::ui::Shutdown();
    Application::Get().SetActiveScene(nullptr);
    Layer::OnDetach();
}

void MainGameLayer::OnUpdate(float ts) {
    Layer::OnUpdate(ts);
    
    // Update navigation system
    if (navSystem_) {
        navSystem_->Update(ts);
    }
    
    scene_->OnUpdate(ts);
    
    // Poll UI input (engine handles hit testing, hover, events)
    se::ui::PollInput();
    
    // Animate health for demo
    if (animateHealth_ && hudController_) {
        static float healthDir = -1.0f;
        demoHealth_ += healthDir * ts * 20.0f;
        
        if (demoHealth_ <= 0.0f) {
            demoHealth_ = 0.0f;
            healthDir = 1.0f;
        } else if (demoHealth_ >= 100.0f) {
            demoHealth_ = 100.0f;
            healthDir = -1.0f;
        }
        
        hudController_->SetHealth(demoHealth_);
    }
    
    // Update native UI layout
    se::ui::Update();
    
    // Debug visualization mode hotkeys (F1-F7)
    auto& renderer = se::Application::Get().GetRenderer().GetSceneRenderer();
    auto& input = se::InputManager::Get();
    if (input.IsKeyDown(se::Key::F1)) renderer.SetDebugMode(0);  // Normal
    if (input.IsKeyDown(se::Key::F2)) renderer.SetDebugMode(1);  // AO
    if (input.IsKeyDown(se::Key::F3)) renderer.SetDebugMode(2);  // Normals
    if (input.IsKeyDown(se::Key::F4)) renderer.SetDebugMode(3);  // Roughness
    if (input.IsKeyDown(se::Key::F5)) renderer.SetDebugMode(4);  // Metallic
    if (input.IsKeyDown(se::Key::F6)) renderer.SetDebugMode(5);  // Depth
    if (input.IsKeyDown(se::Key::F7)) renderer.SetDebugMode(6);  // Geometry
}

void MainGameLayer::OnRender() {
    Layer::OnRender();
    scene_->OnRender();
    
    // Render debug visualization (navigation grid, paths, etc.)
    Camera* camera = scene_->GetActiveCamera();
    if (camera) {
        se::DebugRenderer::Get().Flush(*camera);
        
        // Render World Space UI (enemy names, health bars, etc.)
        se::WorldSpaceUIRenderer::Get().Render(*camera, scene_->GetRegistry());
    }
    
    // Render native UI (engine handles retained-mode, canvas, etc.)
    se::ui::Render();
}

void MainGameLayer::OnImGuiRender() {
    Layer::OnImGuiRender();
    ImguiDebug();
}

void MainGameLayer::SetupHUD() {
    auto& window = se::Application::Get().GetWindow();
    float viewportW = static_cast<float>(window.GetWidth());
    float viewportH = static_cast<float>(window.GetHeight());
    
    // Initialize UI system
    se::ui::Initialize(viewportW, viewportH);
    
    // Create HUD controller
    hudController_ = std::make_unique<se::ui::HUDController>();
    hudController_->Initialize(viewportW, viewportH);
    
    // Configure initial state
    hudController_->SetHealth(100.0f, 100.0f);
    hudController_->SetAbilitySlotCount(3);
    
    // Set up ability slots
    hudController_->SetAbilityKeyLabel(0, "1");
    hudController_->SetAbilityKeyLabel(1, "2");
    hudController_->SetAbilityKeyLabel(2, "SHIFT");
    
    // Get root and set in UI system
    se::ui::SetRoot(hudController_->GetRoot());
    
    // Register resize callback to update HUD positioning
    se::ui::SetOnResizeCallback([this](float width, float height) {
        if (hudController_) {
            hudController_->OnViewportResize(width, height);
        }
    });
    
    SE_LOG_INFO("[MainGameLayer] HUD setup complete with crosshair, health bar, and ability slots");
}

} // namespace FirstGame