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

// Native UI System
#include "engine/ui/native/UIBoxContainer.h"
#include "engine/ui/native/InputEvent.h"
#include "engine/ui/native/theme/UIStyleBox.h"
#include "engine/debug/FrameProfiler.h"

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
    
    // Initialize Native UI Demo
    SetupNativeUI();
}

void MainGameLayer::OnDetach() {
    uiRoot_.reset();  // Clean up UI
    se::ui::Shutdown();
    Application::Get().SetActiveScene(nullptr);
    Layer::OnDetach();
}

void MainGameLayer::OnUpdate(float ts) {
    Layer::OnUpdate(ts);
    scene_->OnUpdate(ts);
    
    // Update native UI input handling
    UpdateNativeUIInput();
    
    // Animate progress bar if enabled
    if (animateProgress_ && progressBar_) {
        progressValue_ += ts * 15.0f;  // 15 units per second
        if (progressValue_ > 100.0f) {
            progressValue_ = 0.0f;
        }
        progressBar_->SetValue(progressValue_);
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
    
    // Render native UI on top
    RenderNativeUI();
}

void MainGameLayer::OnImGuiRender() {
    Layer::OnImGuiRender();
    ImguiDebug();
}

void MainGameLayer::SetupNativeUI() {
    auto& window = se::Application::Get().GetWindow();
    float viewportW = static_cast<float>(window.GetWidth());
    float viewportH = static_cast<float>(window.GetHeight());
    
    // Initialize UI system
    se::ui::Initialize(viewportW, viewportH);
    
    // Create root panel - larger to fit all demo widgets
    auto rootPanel = std::make_unique<se::ui::UIPanel>();
    rootPanel->SetName("UI Demo Root");
    rootPanel->SetPosition({20, 20});
    rootPanel->SetSize({320, 420});
    
    // Style the root panel with a modern dark theme
    auto panelStyle = std::make_shared<se::ui::UIStyleBoxFlat>();
    panelStyle->SetBackgroundColor({0.08f, 0.08f, 0.12f, 0.95f});
    panelStyle->SetBorderWidthAll(2.0f);
    panelStyle->SetBorderColor({0.3f, 0.5f, 0.8f, 1.0f});
    panelStyle->SetCornerRadiusAll(12.0f);
    panelStyle->SetContentMarginAll(12.0f);
    rootPanel->SetStyleBox(panelStyle);
    
    // Create vertical container for layout
    auto vbox = std::make_unique<se::ui::UIVBoxContainer>();
    vbox->SetName("VBox");
    vbox->SetPosition({12, 12});
    vbox->SetSize({296, 396});
    vbox->SetSeparation(10);
    
    // ─────────────────────────────────────────────────────────
    // Title
    // ─────────────────────────────────────────────────────────
    auto titleLabel = std::make_unique<se::ui::UILabel>("Native UI Widget Demo");
    titleLabel->SetName("Title");
    titleLabel->SetFontSize(22.0f);
    titleLabel->SetFontColor({0.5f, 0.85f, 1.0f, 1.0f});
    titleLabel->SetHorizontalAlignment(se::ui::UILabel::HorizontalAlign::CENTER);
    vbox->AddChild(std::move(titleLabel));
    
    // Subtitle
    auto subtitleLabel = std::make_unique<se::ui::UILabel>("Testing new Phase 2 widgets");
    subtitleLabel->SetName("Subtitle");
    subtitleLabel->SetFontSize(12.0f);
    subtitleLabel->SetFontColor({0.6f, 0.6f, 0.7f, 1.0f});
    subtitleLabel->SetHorizontalAlignment(se::ui::UILabel::HorizontalAlign::CENTER);
    vbox->AddChild(std::move(subtitleLabel));
    
    // Separator
    auto separator1 = std::make_unique<se::ui::UIColorRect>(glm::vec4{0.3f, 0.4f, 0.6f, 0.5f});
    separator1->SetName("Separator1");
    separator1->SetCustomMinimumSize({280, 2});
    vbox->AddChild(std::move(separator1));
    
    // ─────────────────────────────────────────────────────────
    // Progress Bar Section
    // ─────────────────────────────────────────────────────────
    auto progressLabel = std::make_unique<se::ui::UILabel>("Progress Bar (animated):");
    progressLabel->SetFontSize(14.0f);
    progressLabel->SetFontColor({0.9f, 0.9f, 0.9f, 1.0f});
    vbox->AddChild(std::move(progressLabel));
    
    auto progressBar = std::make_unique<se::ui::UIProgressBar>();
    progressBar->SetName("DemoProgress");
    progressBar->SetCustomMinimumSize({280, 28});
    progressBar->SetValue(0.0f);
    progressBar->SetFontSize(13.0f);
    
    // Custom progress bar style
    auto progressBg = std::make_shared<se::ui::UIStyleBoxFlat>();
    progressBg->SetBackgroundColor({0.12f, 0.12f, 0.18f, 1.0f});
    progressBg->SetCornerRadiusAll(6.0f);
    progressBar->SetStyleBoxBackground(progressBg);
    
    auto progressFill = std::make_shared<se::ui::UIStyleBoxFlat>();
    progressFill->SetBackgroundColor({0.2f, 0.6f, 0.4f, 1.0f});
    progressFill->SetCornerRadiusAll(6.0f);
    progressBar->SetStyleBoxFill(progressFill);
    
    progressBar_ = progressBar.get();
    vbox->AddChild(std::move(progressBar));
    
    // ─────────────────────────────────────────────────────────
    // Slider Section
    // ─────────────────────────────────────────────────────────
    auto sliderLabel = std::make_unique<se::ui::UILabel>("Slider (drag to adjust):");
    sliderLabel->SetFontSize(14.0f);
    sliderLabel->SetFontColor({0.9f, 0.9f, 0.9f, 1.0f});
    vbox->AddChild(std::move(sliderLabel));
    
    // Slider value label
    auto sliderValueLabel = std::make_unique<se::ui::UILabel>("Value: 50");
    sliderValueLabel->SetFontSize(12.0f);
    sliderValueLabel->SetFontColor({0.8f, 0.9f, 0.6f, 1.0f});
    sliderValueLabel_ = sliderValueLabel.get();
    vbox->AddChild(std::move(sliderValueLabel));
    
    auto slider = std::make_unique<se::ui::UIHSlider>();
    slider->SetName("DemoSlider");
    slider->SetCustomMinimumSize({280, 28});
    slider->SetRange(0.0f, 100.0f);
    slider->SetValue(50.0f);
    slider->SetStep(1.0f);
    slider->SetGrabberSize({18.0f, 26.0f});
    
    // Custom slider track style
    auto sliderTrack = std::make_shared<se::ui::UIStyleBoxFlat>();
    sliderTrack->SetBackgroundColor({0.15f, 0.15f, 0.2f, 1.0f});
    sliderTrack->SetCornerRadiusAll(3.0f);
    slider->SetStyleBoxTrack(sliderTrack);
    
    // Slider grabber style
    auto sliderGrabber = std::make_shared<se::ui::UIStyleBoxFlat>();
    sliderGrabber->SetBackgroundColor({0.4f, 0.55f, 0.8f, 1.0f});
    sliderGrabber->SetCornerRadiusAll(5.0f);
    sliderGrabber->SetBorderWidthAll(1.0f);
    sliderGrabber->SetBorderColor({0.5f, 0.65f, 0.9f, 1.0f});
    slider->SetStyleBoxGrabber(sliderGrabber);
    
    // Slider grabber highlight style
    auto sliderGrabberHL = std::make_shared<se::ui::UIStyleBoxFlat>();
    sliderGrabberHL->SetBackgroundColor({0.5f, 0.65f, 0.9f, 1.0f});
    sliderGrabberHL->SetCornerRadiusAll(5.0f);
    sliderGrabberHL->SetBorderWidthAll(2.0f);
    sliderGrabberHL->SetBorderColor({0.6f, 0.75f, 1.0f, 1.0f});
    slider->SetStyleBoxGrabberHighlight(sliderGrabberHL);
    
    slider->SetOnValueChanged([this](float value) {
        if (sliderValueLabel_) {
            sliderValueLabel_->SetText("Value: " + std::to_string(static_cast<int>(value)));
        }
    });
    
    vbox->AddChild(std::move(slider));
    
    // ─────────────────────────────────────────────────────────
    // Checkbox Section
    // ─────────────────────────────────────────────────────────
    auto checkboxLabel = std::make_unique<se::ui::UILabel>("Checkboxes:");
    checkboxLabel->SetFontSize(14.0f);
    checkboxLabel->SetFontColor({0.9f, 0.9f, 0.9f, 1.0f});
    vbox->AddChild(std::move(checkboxLabel));
    
    // Animate progress checkbox
    auto animateCheckbox = std::make_unique<se::ui::UICheckBox>("Animate Progress Bar");
    animateCheckbox->SetName("AnimateCheckbox");
    animateCheckbox->SetChecked(true);
    animateCheckbox->SetFontSize(13.0f);
    animateCheckbox->SetOnToggled([this](bool checked) {
        animateProgress_ = checked;
        SE_LOG_INFO("[UI Demo] Animate progress: {}", checked ? "ON" : "OFF");
    });
    vbox->AddChild(std::move(animateCheckbox));
    
    // Another checkbox to demonstrate
    auto optionCheckbox = std::make_unique<se::ui::UICheckBox>("Enable Option B");
    optionCheckbox->SetName("OptionCheckbox");
    optionCheckbox->SetChecked(false);
    optionCheckbox->SetFontSize(13.0f);
    vbox->AddChild(std::move(optionCheckbox));
    
    // Separator 2
    auto separator2 = std::make_unique<se::ui::UIColorRect>(glm::vec4{0.3f, 0.4f, 0.6f, 0.5f});
    separator2->SetName("Separator2");
    separator2->SetCustomMinimumSize({280, 2});
    vbox->AddChild(std::move(separator2));
    
    // ─────────────────────────────────────────────────────────
    // Button Section
    // ─────────────────────────────────────────────────────────
    
    // Click counter label
    auto clickLabel = std::make_unique<se::ui::UILabel>("Button clicks: 0");
    clickLabel->SetName("ClickCounter");
    clickLabel->SetFontSize(14.0f);
    clickLabel->SetFontColor({1.0f, 0.9f, 0.5f, 1.0f});
    auto* clickLabelPtr = clickLabel.get();
    vbox->AddChild(std::move(clickLabel));
    
    // Button styles
    auto btnNormal = std::make_shared<se::ui::UIStyleBoxFlat>();
    btnNormal->SetBackgroundColor({0.25f, 0.45f, 0.7f, 1.0f});
    btnNormal->SetBorderWidthAll(1.0f);
    btnNormal->SetBorderColor({0.35f, 0.55f, 0.8f, 1.0f});
    btnNormal->SetCornerRadiusAll(8.0f);
    btnNormal->SetContentMarginAll(10.0f);
    
    auto btnHover = std::make_shared<se::ui::UIStyleBoxFlat>();
    btnHover->SetBackgroundColor({0.35f, 0.55f, 0.8f, 1.0f});
    btnHover->SetBorderWidthAll(2.0f);
    btnHover->SetBorderColor({0.45f, 0.65f, 0.9f, 1.0f});
    btnHover->SetCornerRadiusAll(8.0f);
    btnHover->SetContentMarginAll(10.0f);
    
    auto btnPressed = std::make_shared<se::ui::UIStyleBoxFlat>();
    btnPressed->SetBackgroundColor({0.18f, 0.38f, 0.6f, 1.0f});
    btnPressed->SetBorderWidthAll(2.0f);
    btnPressed->SetBorderColor({0.28f, 0.48f, 0.7f, 1.0f});
    btnPressed->SetCornerRadiusAll(8.0f);
    btnPressed->SetContentMarginAll(10.0f);
    
    // Create main button
    auto button = std::make_unique<se::ui::UIButton>("Click Me!");
    button->SetName("TestButton");
    button->SetCustomMinimumSize({140, 36});
    button->SetFontSize(16.0f);
    button->SetStyleBoxNormal(btnNormal);
    button->SetStyleBoxHover(btnHover);
    button->SetStyleBoxPressed(btnPressed);
    
    button->SetOnReleased([this, clickLabelPtr]() {
        buttonClickCount_++;
        clickLabelPtr->SetText("Button clicks: " + std::to_string(buttonClickCount_));
        SE_LOG_INFO("[UI Demo] Button clicked! Count: {}", buttonClickCount_);
    });
    
    vbox->AddChild(std::move(button));
    
    // Get pointer before moving
    auto* vboxPtr = vbox.get();
    rootPanel->AddChild(std::move(vbox));
    
    // Trigger initial layout
    vboxPtr->SortChildren();
    
    uiRoot_ = std::move(rootPanel);
    se::ui::SetRoot(std::move(std::make_unique<se::ui::UIControl>()));  // Dummy root for now
    
    SE_LOG_INFO("[UI Demo] Native UI setup complete with Phase 2 widgets");
}

void MainGameLayer::RenderNativeUI() {
    SE_PROFILE_SCOPE_COLOR("NativeUI", ProfilerColors::Carrot);
    if (!uiRoot_) return;
    
    auto& canvas = se::ui::UICanvas2D::Get();
    auto& window = se::Application::Get().GetWindow();
    
    canvas.SetViewport(
        static_cast<float>(window.GetWidth()),
        static_cast<float>(window.GetHeight())
    );
    
    // Check if any control needs redraw (retained mode optimization)
    std::function<bool(se::ui::UIControl*)> checkNeedsRedraw = [&](se::ui::UIControl* ctrl) -> bool {
        if (!ctrl || !ctrl->IsVisible()) return false;
        if (ctrl->NeedsRedraw()) return true;
        for (const auto& child : ctrl->GetChildren()) {
            if (checkNeedsRedraw(child.get())) return true;
        }
        return false;
    };
    
    bool anyNeedsRedraw = checkNeedsRedraw(uiRoot_.get());
    
    // Only regenerate commands if something changed
    if (anyNeedsRedraw || !canvas.HasCachedCommands()) {
        SE_PROFILE_SCOPE_COLOR("NativeUI::BuildCommands", ProfilerColors::Alizarin);
        canvas.BeginFrame();
        
        // Recursively draw entire UI tree
        std::function<void(se::ui::UIControl*)> drawControl = [&](se::ui::UIControl* ctrl) {
            if (!ctrl || !ctrl->IsVisible()) return;
            
            ctrl->Draw();
            ctrl->ClearRedrawFlag();
            
            for (const auto& child : ctrl->GetChildren()) {
                drawControl(child.get());
            }
        };
        
        drawControl(uiRoot_.get());
        canvas.EndFrame();
    }
    
    {
        SE_PROFILE_SCOPE_COLOR("NativeUI::Render", ProfilerColors::Amethyst);
        canvas.Render();
    }
}

void MainGameLayer::UpdateNativeUIInput() {
    if (!uiRoot_) return;
    
    auto& input = se::InputManager::Get();
    
    // Get current mouse state
    glm::vec2 mousePos = input.GetMousePosition();
    bool mousePressed = input.IsMouseButtonDown(0);  // Left button
    
    // Calculate delta
    glm::vec2 mouseDelta = mousePos - lastMousePos_;
    
    // Hit test and dispatch to controls
    std::function<se::ui::UIControl*(se::ui::UIControl*, const glm::vec2&)> hitTest = 
        [&hitTest](se::ui::UIControl* ctrl, const glm::vec2& pos) -> se::ui::UIControl* {
        if (!ctrl || !ctrl->IsVisible()) return nullptr;
        
        // Check children first (reverse order for front-to-back)
        const auto& children = ctrl->GetChildren();
        for (auto it = children.rbegin(); it != children.rend(); ++it) {
            if (auto* hit = hitTest(it->get(), pos)) {
                return hit;
            }
        }
        
        // Check this control
        glm::vec2 globalPos = ctrl->GetGlobalPosition();
        glm::vec2 size = ctrl->GetSize();
        if (pos.x >= globalPos.x && pos.x < globalPos.x + size.x &&
            pos.y >= globalPos.y && pos.y < globalPos.y + size.y) {
            if (ctrl->GetMouseFilter() != se::ui::MouseFilter::MOUSE_IGNORE) {
                return ctrl;
            }
        }
        return nullptr;
    };
    
    se::ui::UIControl* hoveredControl = hitTest(uiRoot_.get(), mousePos);
    
    // Track hover state changes and send MOUSE_ENTER/MOUSE_EXIT notifications
    if (hoveredControl != lastHoveredControl_) {
        if (lastHoveredControl_) {
            lastHoveredControl_->OnNotification(se::ui::ControlNotification::MOUSE_EXIT);
        }
        if (hoveredControl) {
            hoveredControl->OnNotification(se::ui::ControlNotification::MOUSE_ENTER);
        }
        lastHoveredControl_ = hoveredControl;
    }
    
    // Generate and dispatch mouse motion events
    if (mouseDelta.x != 0.0f || mouseDelta.y != 0.0f) {
        auto motionEvent = se::ui::InputEvent::MouseMotion(mousePos, mouseDelta);
        if (hoveredControl) {
            hoveredControl->OnInput(motionEvent);
        }
    }
    
    // Mouse button state changes
    if (mousePressed && !lastMousePressed_) {
        auto pressEvent = se::ui::InputEvent::MouseButtonPressed(
            se::ui::MouseButton::LEFT, mousePos);
        if (hoveredControl) {
            hoveredControl->OnInput(pressEvent);
        }
    } else if (!mousePressed && lastMousePressed_) {
        auto releaseEvent = se::ui::InputEvent::MouseButtonReleased(
            se::ui::MouseButton::LEFT, mousePos);
        if (hoveredControl) {
            hoveredControl->OnInput(releaseEvent);
        }
    }
    
    // Update state for next frame
    lastMousePos_ = mousePos;
    lastMousePressed_ = mousePressed;
}

} // namespace FirstGame