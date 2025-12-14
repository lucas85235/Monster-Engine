#include "NarratorLayer.h"
#include "engine/ai/AIManager.h"
#include "engine/Application.h"
#include "engine/Log.h"
#include "engine/input/InputManager.h"
#include "engine/input/KeyCodes.h"
#include "engine/events/KeyEvent.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/physics/RigidbodyComponent.h"
#include "engine/physics/BoxCollider.h"
#include "engine/resources/MeshManager.h"
#include "engine/audio/TTSManager.h"
#include "../SampleUtilities.h"

#include <imgui.h>
#include <thread>

NarratorLayer::NarratorLayer() : Layer("NarratorLayer") {
    SE_LOG_INFO("[NarratorLayer] Constructed");
}

NarratorLayer::~NarratorLayer() {
    SE_LOG_INFO("[NarratorLayer] Destroyed");
}

void NarratorLayer::OnAttach() {
    SE_LOG_INFO("[NarratorLayer] OnAttach - Setting up game world...");
    
    scene_ = CreateScope<Scene>();
    
    SetupInputBindings();
    SetupScene();
    SetupTriggers();
    
    // Initialize TTS
    if (!se::TTSManager::Get().Initialize()) {
        SE_LOG_WARN("[NarratorLayer] TTS initialization failed, narrator will be silent");
    }
    
    // Initialize AI
    std::string model_path = std::string(PROJECT_SOURCE_DIR) + "/engine/models/" + config_.model_file;
    
    se::AIConfig ai_config;
    ai_config.model_path = model_path;
    ai_config.n_ctx = config_.context_size;
    ai_config.n_threads = config_.max_threads;
    ai_config.n_gpu_layers = config_.gpu_layers;
    ai_config.temperature = config_.temperature;
    ai_config.top_p = config_.top_p;
    ai_config.top_k = config_.top_k;
    ai_config.min_p = config_.min_p;
    ai_config.penalty_repeat = config_.penalty_repeat;
    ai_config.penalty_last_n = config_.penalty_last_n;
    ai_config.dry_multiplier = config_.dry_multiplier;
    
    ai_loading_ = true;
    status_text_ = language_ == Language::English ? "Loading Narrator AI..." : "Carregando Narrador IA...";
    
    std::thread init_thread([this, ai_config]() {
        bool success = se::AIManager::Get().Initialize(ai_config);
        ai_loading_ = false;
        if (success) {
            ai_initialized_ = true;
            status_text_ = language_ == Language::English ? "Narrator Ready" : "Narrador Pronto";
            SE_LOG_INFO("[NarratorLayer] Narrator AI initialized");
        } else {
            status_text_ = language_ == Language::English ? "Failed to initialize AI" : "Falha ao inicializar IA";
            SE_LOG_ERROR("[NarratorLayer] Failed to initialize Narrator AI");
        }
    });
    init_thread.detach();
}

void NarratorLayer::SetupInputBindings() {
    auto& input = InputManager::Get();
    
    // Movement - each axis needs two bindings (positive and negative)
    input.BindAxis("MoveForward", Key::W, 1.0f);
    input.BindAxis("MoveForward", Key::S, -1.0f);
    input.BindAxis("MoveRight", Key::D, 1.0f);
    input.BindAxis("MoveRight", Key::A, -1.0f);
    input.BindAxis("MoveUp", Key::Space, 1.0f);
    input.BindAxis("MoveUp", Key::LeftControl, -1.0f);
    
    // Mouse look axes (Y inverted for natural look)
    input.BindAxis("LookX", Key::MouseX, 1.0f);
    input.BindAxis("LookY", Key::MouseY, -1.0f);  // Inverted Y
    
    SE_LOG_INFO("[NarratorLayer] Input bindings configured");
}

void NarratorLayer::SetupScene() {
    material_ = Utilities::LoadMaterial();
    
    // Camera setup - first person view
    camera_ = Camera(glm::vec3(0.0f, 1.7f, 0.0f));  // Eye height
    camera_.SetPitch(0.0f);
    camera_.SetYaw(-90.0f);  // Looking forward (-Z)
    
    camera_controller_ = CreateScope<CameraController>(camera_);
    camera_controller_->SetSpeed(5.0f);
    camera_controller_->SetSensitivity(0.1f);
    
    // Floor
    {
        auto floor = scene_->CreateEntity("Floor");
        auto mesh = MeshManager::GetPrimitive(PrimitiveMeshType::Cube);
        floor.AddComponent<MeshRenderComponent>(mesh, material_);
        
        auto& transform = floor.GetComponent<TransformComponent>();
        transform.SetPosition({0.0f, -0.5f, -15.0f});
        transform.SetScale({10.0f, 1.0f, 40.0f});
        
        // Size={1,1,1} because Scale is applied by PhysicsSystem
        BoxCollider collider;
        collider.Size = {1.0f, 1.0f, 1.0f};
        floor.AddComponent<BoxCollider>(collider);
        
        RigidbodyData data = RigidbodyData{.mass = 0.0f};
        floor.AddComponent<RigidbodyComponent>(data, floor);
    }
    
    // Left wall
    {
        auto wall = scene_->CreateEntity("LeftWall");
        auto mesh = MeshManager::GetPrimitive(PrimitiveMeshType::Cube);
        wall.AddComponent<MeshRenderComponent>(mesh, material_);
        
        auto& transform = wall.GetComponent<TransformComponent>();
        transform.SetPosition({-5.0f, 2.0f, -15.0f});
        transform.SetScale({0.5f, 5.0f, 40.0f});
        
        BoxCollider collider;
        collider.Size = {1.0f, 1.0f, 1.0f};
        wall.AddComponent<BoxCollider>(collider);
        
        RigidbodyData data = RigidbodyData{.mass = 0.0f};
        wall.AddComponent<RigidbodyComponent>(data, wall);
    }
    
    // Right wall
    {
        auto wall = scene_->CreateEntity("RightWall");
        auto mesh = MeshManager::GetPrimitive(PrimitiveMeshType::Cube);
        wall.AddComponent<MeshRenderComponent>(mesh, material_);
        
        auto& transform = wall.GetComponent<TransformComponent>();
        transform.SetPosition({5.0f, 2.0f, -15.0f});
        transform.SetScale({0.5f, 5.0f, 40.0f});
        
        BoxCollider collider;
        collider.Size = {1.0f, 1.0f, 1.0f};
        wall.AddComponent<BoxCollider>(collider);
        
        RigidbodyData data = RigidbodyData{.mass = 0.0f};
        wall.AddComponent<RigidbodyComponent>(data, wall);
    }
    
    // End wall with bifurcation (wall with gap in middle)
    {
        // Left section of end wall
        auto wall_left = scene_->CreateEntity("EndWallLeft");
        auto mesh = MeshManager::GetPrimitive(PrimitiveMeshType::Cube);
        wall_left.AddComponent<MeshRenderComponent>(mesh, material_);
        
        auto& transform = wall_left.GetComponent<TransformComponent>();
        transform.SetPosition({-3.5f, 2.0f, -30.0f});
        transform.SetScale({3.0f, 5.0f, 0.5f});
        
        BoxCollider collider;
        collider.Size = {1.0f, 1.0f, 1.0f};
        wall_left.AddComponent<BoxCollider>(collider);
        
        RigidbodyData data = RigidbodyData{.mass = 0.0f};
        wall_left.AddComponent<RigidbodyComponent>(data, wall_left);
    }
    
    {
        // Right section of end wall
        auto wall_right = scene_->CreateEntity("EndWallRight");
        auto mesh = MeshManager::GetPrimitive(PrimitiveMeshType::Cube);
        wall_right.AddComponent<MeshRenderComponent>(mesh, material_);
        
        auto& transform = wall_right.GetComponent<TransformComponent>();
        transform.SetPosition({3.5f, 2.0f, -30.0f});
        transform.SetScale({3.0f, 5.0f, 0.5f});
        
        BoxCollider collider;
        collider.Size = {1.0f, 1.0f, 1.0f};
        wall_right.AddComponent<BoxCollider>(collider);
        
        RigidbodyData data = RigidbodyData{.mass = 0.0f};
        wall_right.AddComponent<RigidbodyComponent>(data, wall_right);
    }
    
    // Directional Light
    {
        auto light = scene_->CreateEntity("Sun");
        auto& dirLight = light.AddComponent<DirectionalLightComponent>();
        dirLight.Color = {1.0f, 0.95f, 0.9f};
        dirLight.Intensity = 1.0f;
        
        auto& transform = light.GetComponent<TransformComponent>();
        transform.SetRotation({45.0f, 45.0f, 0.0f});
    }
    
    // Player entity with physics
    {
        player_entity_ = scene_->CreateEntity("Player");
        
        auto& transform = player_entity_.GetComponent<TransformComponent>();
        transform.SetPosition({0.0f, 2.0f, 0.0f});  // Start above ground
        
        // Capsule collider for player
        CapsuleCollider collider;
        collider.Radius = 0.4f;
        collider.Height = 1.0f;
        player_entity_.AddComponent<CapsuleCollider>(collider);
        
        // Rigidbody with mass for gravity
        RigidbodyData data;
        data.mass = 70.0f;  // 70kg player
        data.freezeRotationX = true;  // Prevent tumbling
        data.freezeRotationY = true;
        data.freezeRotationZ = true;
        player_entity_.AddComponent<RigidbodyComponent>(data, player_entity_);
    }
    
    SE_LOG_INFO("[NarratorLayer] Scene setup complete");
}

void NarratorLayer::SetupTriggers() {
    // Trigger 1: First steps (near start)
    triggers_.push_back({
        .name = "corridor_start",
        .event = GameEvent::PlayerStartedWalking,
        .position = {0.0f, 1.0f, -5.0f},
        .size = {8.0f, 3.0f, 2.0f},
        .triggered = false,
        .one_shot = true
    });
    
    // Trigger 2: Bifurcation point
    triggers_.push_back({
        .name = "bifurcation",
        .event = GameEvent::PlayerReachedBifurcation,
        .position = {0.0f, 1.0f, -28.0f},
        .size = {4.0f, 3.0f, 2.0f},
        .triggered = false,
        .one_shot = true
    });
    
    // Trigger 3: Went left
    triggers_.push_back({
        .name = "went_left",
        .event = GameEvent::PlayerWentLeft,
        .position = {-3.0f, 1.0f, -32.0f},
        .size = {3.0f, 3.0f, 2.0f},
        .triggered = false,
        .one_shot = true
    });
    
    // Trigger 4: Went right
    triggers_.push_back({
        .name = "went_right",
        .event = GameEvent::PlayerWentRight,
        .position = {3.0f, 1.0f, -32.0f},
        .size = {3.0f, 3.0f, 2.0f},
        .triggered = false,
        .one_shot = true
    });
    
    SE_LOG_INFO("[NarratorLayer] {} triggers configured", triggers_.size());
}

void NarratorLayer::OnDetach() {
    SE_LOG_INFO("[NarratorLayer] OnDetach");
    se::TTSManager::Get().Shutdown();
    se::AIManager::Get().Shutdown();
}

void NarratorLayer::OnUpdate(float ts) {
    se::AIManager::Get().ProcessCallbacks();
    
    if (game_mode_) {
        UpdatePlayer(ts);
        CheckTriggers();
    }
    
    scene_->OnUpdate(ts);
}

void NarratorLayer::UpdatePlayer(float ts) {
    if (!player_entity_ || !player_entity_.HasComponent<RigidbodyComponent>()) {
        // Fallback to camera-only control if no physics
        if (camera_controller_) {
            camera_controller_->OnUpdate(ts);
        }
        return;
    }
    
    auto& input = InputManager::Get();
    auto& rb = player_entity_.GetComponent<RigidbodyComponent>();
    auto& transform = player_entity_.GetComponent<TransformComponent>();
    
    // Get movement input
    float moveForward = input.GetAxis("MoveForward");
    float moveRight = input.GetAxis("MoveRight");
    
    // Get camera direction for movement (only horizontal)
    glm::vec3 camForward = camera_.GetFront();
    camForward.y = 0.0f;
    camForward = glm::normalize(camForward);
    
    glm::vec3 camRight = camera_.GetRight();
    camRight.y = 0.0f;
    camRight = glm::normalize(camRight);
    
    // Calculate desired movement velocity
    float moveSpeed = 5.0f;
    glm::vec3 moveDir = camForward * moveForward + camRight * moveRight;
    
    if (glm::length(moveDir) > 0.01f) {
        moveDir = glm::normalize(moveDir) * moveSpeed;
    }
    
    // Apply horizontal velocity while preserving vertical (gravity)
    btVector3 currentVel = rb.GetLinearVelocity();
    rb.SetLinearVelocity(btVector3(moveDir.x, currentVel.y(), moveDir.z));
    
    // Update camera rotation from mouse
    float lookX = input.GetAxis("LookX");
    float lookY = input.GetAxis("LookY");
    
    if (lookX != 0.0f || lookY != 0.0f) {
        camera_.ProcessMouseMovement(lookX, lookY);
    }
    
    // Sync camera position with player (at eye height)
    glm::vec3 playerPos = transform.Position;
    float eyeHeight = 1.6f;  // Eye height from player center
    camera_.SetPosition(glm::vec3(playerPos.x, playerPos.y + eyeHeight, playerPos.z));
}

void NarratorLayer::CheckTriggers() {
    glm::vec3 player_pos = camera_.GetPosition();
    
    for (auto& trigger : triggers_) {
        if (trigger.triggered && trigger.one_shot) {
            continue;
        }
        
        // Simple AABB check
        glm::vec3 min = trigger.position - trigger.size * 0.5f;
        glm::vec3 max = trigger.position + trigger.size * 0.5f;
        
        bool inside = player_pos.x >= min.x && player_pos.x <= max.x &&
                      player_pos.y >= min.y && player_pos.y <= max.y &&
                      player_pos.z >= min.z && player_pos.z <= max.z;
        
        if (inside && !trigger.triggered) {
            trigger.triggered = true;
            triggered_zones_.insert(trigger.name);
            
            SE_LOG_INFO("[NarratorLayer] Trigger activated: {}", trigger.name);
            SendEvent(trigger.event);
        }
    }
}

void NarratorLayer::OnRender() {
    auto& window = Application::Get().GetWindow();
    float aspectRatio = static_cast<float>(window.GetWidth()) / static_cast<float>(window.GetHeight());
    
    scene_->OnRender(camera_, aspectRatio);
}

void NarratorLayer::OnImGuiRender() {
    RenderGameUI();
    RenderNarratorWindow();
    RenderDebugWindow();
}

void NarratorLayer::OnEvent(Event& event) {
    // Toggle game mode with Tab
    if (event.GetEventType() == se::EventType::KeyPressed) {
        auto& keyEvent = static_cast<KeyPressedEvent&>(event);
        if (keyEvent.GetKeyCode() == Key::Tab) {
            game_mode_ = !game_mode_;
            
            if (game_mode_) {
                InputManager::Get().SetCursorMode(CursorMode::Locked);
                status_text_ = language_ == Language::English ? "Game Mode (Tab to exit)" : "Modo Jogo (Tab para sair)";
            } else {
                InputManager::Get().SetCursorMode(CursorMode::Normal);
                status_text_ = language_ == Language::English ? "Menu Mode (Tab to play)" : "Modo Menu (Tab para jogar)";
            }
        }
    }
}

void NarratorLayer::RenderGameUI() {
    ImGui::SetNextWindowPos(ImVec2(10, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(300, 100), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("Controls", nullptr, ImGuiWindowFlags_NoCollapse);
    
    RenderLanguageSelector();
    
    ImGui::Separator();
    
    if (ai_loading_) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "%s", 
            (language_ == Language::English ? "Loading AI..." : "Carregando IA..."));
    } else if (ai_initialized_) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "%s",
            (language_ == Language::English ? "AI Ready" : "IA Pronta"));
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "%s",
            (language_ == Language::English ? "AI Not Ready" : "IA Nao Pronta"));
    }
    
    ImGui::Text("%s", status_text_.c_str());
    
    ImGui::Separator();
    
    if (game_mode_) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.5f, 1.0f), "%s",
            (language_ == Language::English ? "PLAYING - Press TAB to exit" : "JOGANDO - Pressione TAB para sair"));
    } else {
        if (ImGui::Button(language_ == Language::English ? "Start Game (or press TAB)" : "Iniciar Jogo (ou pressione TAB)")) {
            game_mode_ = true;
            InputManager::Get().SetCursorMode(CursorMode::Locked);
        }
    }
    
    ImGui::Separator();
    
    // Controls section
    bool en = (language_ == Language::English);
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.3f, 1.0f), "%s", en ? "CONTROLS:" : "CONTROLES:");
    ImGui::BulletText("W/S: %s", en ? "Move Forward/Back" : "Mover Frente/Tras");
    ImGui::BulletText("A/D: %s", en ? "Move Left/Right" : "Mover Esquerda/Direita");
    ImGui::BulletText("Mouse: %s", en ? "Look Around" : "Olhar");
    ImGui::BulletText("TAB: %s", en ? "Toggle Menu" : "Alternar Menu");
    
    ImGui::End();
}

void NarratorLayer::RenderLanguageSelector() {
    ImGui::Text("%s", (language_ == Language::English ? "Language:" : "Idioma:"));
    ImGui::SameLine();
    
    bool is_english = (language_ == Language::English);
    if (ImGui::RadioButton("EN", is_english)) {
        language_ = Language::English;
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("PT", !is_english)) {
        language_ = Language::Portuguese;
    }
}

void NarratorLayer::RenderNarratorWindow() {
    ImGui::SetNextWindowPos(ImVec2(10, 400), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(600, 200), ImGuiCond_FirstUseEver);
    
    std::string title = language_ == Language::English ? "Narrator" : "Narrador";
    ImGui::Begin(title.c_str(), nullptr, ImGuiWindowFlags_NoCollapse);
    
    ImGui::BeginChild("Messages", ImVec2(0, -30), true);
    
    for (const auto& msg : messages_) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.7f, 1.0f, 1.0f));
        ImGui::TextWrapped("[%s] %s", 
            (language_ == Language::English ? "Event" : "Evento"),
            msg.event_name.c_str());
        ImGui::PopStyleColor();
        
        if (msg.is_pending) {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::TextWrapped("%s", 
                (language_ == Language::English ? "Narrator is thinking..." : "Narrador esta pensando..."));
            ImGui::PopStyleColor();
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.6f, 1.0f));
            ImGui::TextWrapped("[%s] %s",
                (language_ == Language::English ? "Narrator" : "Narrador"),
                msg.response.c_str());
            ImGui::PopStyleColor();
        }
        
        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
    }
    
    if (ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    
    ImGui::EndChild();
    
    // Clear button
    if (ImGui::Button(language_ == Language::English ? "Clear" : "Limpar")) {
        messages_.clear();
        triggered_zones_.clear();
        for (auto& t : triggers_) {
            t.triggered = false;
        }
    }
    
    // TTS controls
    ImGui::SameLine();
    ImGui::Text(" | TTS:");
    ImGui::SameLine();
    
    auto& tts = se::TTSManager::Get();
    if (tts.IsInitialized()) {
        if (tts.IsSpeaking() || tts.IsPaused()) {
            // Speaking or paused - show pause/resume and stop
            if (tts.IsPaused()) {
                if (ImGui::Button(language_ == Language::English ? "Resume" : "Continuar")) {
                    tts.Resume();
                }
            } else {
                if (ImGui::Button(language_ == Language::English ? "Pause" : "Pausar")) {
                    tts.Pause();
                }
            }
            ImGui::SameLine();
            if (ImGui::Button("Stop")) {
                tts.Stop();
            }
        } else {
            ImGui::TextColored(ImVec4(0.5f, 0.5f, 0.5f, 1.0f), "%s", 
                language_ == Language::English ? "(Idle)" : "(Parado)");
        }
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.3f, 0.3f, 1.0f), "%s",
            language_ == Language::English ? "N/A" : "N/D");
    }
    
    ImGui::End();
}

void NarratorLayer::RenderDebugWindow() {
    ImGui::SetNextWindowPos(ImVec2(620, 10), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(250, 150), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("Debug", nullptr, ImGuiWindowFlags_NoCollapse);
    
    glm::vec3 pos = camera_.GetPosition();
    ImGui::Text("Position: %.1f, %.1f, %.1f", pos.x, pos.y, pos.z);
    ImGui::Text("Triggers activated: %zu/%zu", triggered_zones_.size(), triggers_.size());
    ImGui::Text("Game mode: %s", game_mode_ ? "ON" : "OFF");
    
    ImGui::End();
}

std::string NarratorLayer::GetEventDescription(GameEvent event) {
    bool en = (language_ == Language::English);
    
    switch (event) {
        case GameEvent::PlayerStartedWalking:
            return en ? "The player started walking down the corridor" 
                      : "O jogador comecou a andar pelo corredor";
        
        case GameEvent::PlayerReachedBifurcation:
            return en ? "The player reached the bifurcation - two paths lay ahead, left and right"
                      : "O jogador chegou na bifurcacao - dois caminhos a frente, esquerda e direita";
        
        case GameEvent::PlayerWentLeft:
            return en ? "The player chose to go LEFT"
                      : "O jogador escolheu ir para a ESQUERDA";
        
        case GameEvent::PlayerWentRight:
            return en ? "The player chose to go RIGHT"
                      : "O jogador escolheu ir para a DIREITA";
        
        case GameEvent::PlayerEnteredLeftRoom:
            return en ? "The player entered the left room"
                      : "O jogador entrou na sala da esquerda";
        
        case GameEvent::PlayerEnteredRightRoom:
            return en ? "The player entered the right room"
                      : "O jogador entrou na sala da direita";
        
        case GameEvent::PlayerReturnedToStart:
            return en ? "The player returned to the starting point"
                      : "O jogador voltou ao ponto inicial";
        
        case GameEvent::PlayerOpenedDoor:
            return en ? "The player opened a door"
                      : "O jogador abriu uma porta";
        
        case GameEvent::PlayerFoundSecret:
            return en ? "The player discovered a secret"
                      : "O jogador descobriu um segredo";
        
        case GameEvent::NarratorSaidGoLeft:
            return en ? "[INSTRUCTION] Tell the player to go left"
                      : "[INSTRUCAO] Diga ao jogador para ir para a esquerda";
        
        case GameEvent::NarratorSaidGoRight:
            return en ? "[INSTRUCTION] Tell the player to go right"
                      : "[INSTRUCAO] Diga ao jogador para ir para a direita";
        
        case GameEvent::PlayerObeyedCommand:
            return en ? "The player obeyed your instruction"
                      : "O jogador obedeceu sua instrucao";
        
        case GameEvent::PlayerDisobeyedCommand:
            return en ? "The player disobeyed your instruction and did the opposite"
                      : "O jogador desobedeceu sua instrucao e fez o oposto";
        
        default:
            return en ? "Something happened" : "Algo aconteceu";
    }
}

std::string NarratorLayer::GetFrustrationLevel(int count) {
    bool en = (language_ == Language::English);
    
    if (count <= 1) {
        return "";
    } else if (count == 2) {
        return en ? "[REPETITION #2] " : "[REPETICAO #2] ";
    } else if (count == 3) {
        return en ? "[REPETITION #3 - getting annoyed] " : "[REPETICAO #3 - ficando irritado] ";
    } else if (count <= 5) {
        return en ? "[REPETITION #" + std::to_string(count) + " - frustrated] "
                  : "[REPETICAO #" + std::to_string(count) + " - frustrado] ";
    } else {
        return en ? "[REPETITION #" + std::to_string(count) + " - losing patience] "
                  : "[REPETICAO #" + std::to_string(count) + " - perdendo paciencia] ";
    }
}

void NarratorLayer::SendEvent(GameEvent event, const std::string& custom_event) {
    if (!ai_initialized_ || waiting_response_) return;
    
    std::string event_desc = (event == GameEvent::Custom) ? custom_event : GetEventDescription(event);
    
    int event_key = (event == GameEvent::Custom) ? -1 : static_cast<int>(event);
    event_counts_[event_key]++;
    int repetition_count = event_counts_[event_key];
    
    messages_.push_back({event_desc, "", repetition_count, true});
    waiting_response_ = true;
    status_text_ = language_ == Language::English ? "Narrator is thinking..." : "Narrador esta pensando...";
    
    std::string frustration = GetFrustrationLevel(repetition_count);
    
    // Build history
    std::string event_history = "";
    int history_count = 0;
    for (int i = static_cast<int>(messages_.size()) - 2; i >= 0 && history_count < 5; --i) {
        const auto& msg = messages_[i];
        if (!msg.is_pending && !msg.response.empty()) {
            event_history = "- " + msg.event_name + " -> You said: \"" + msg.response + "\"\n" + event_history;
            history_count++;
        }
    }
    
    // Build narrator prompt - Stanley Parable style
    bool en = (language_ == Language::English);
    
    std::string narrator_prompt;
    
    if (en) {
        narrator_prompt = 
            "You are the Narrator from a game like The Stanley Parable. You narrate what happens "
            "in a detached, omniscient, slightly amused tone. You describe actions in third person "
            "and occasionally address the player directly with a hint of passive-aggressiveness.\n\n"
            
            "STYLE GUIDELINES:\n"
            "- Speak in third person about 'the player' or 'Stanley' (you can use either)\n"
            "- Be articulate, sophisticated, and occasionally sarcastic\n"
            "- React to player choices with veiled disappointment or surprise\n"
            "- Keep responses focused on the current action (1-3 sentences)\n"
            "- Your tone should evolve based on what happened before\n\n";
    } else {
        narrator_prompt = 
            "Voce e o Narrador de um jogo como The Stanley Parable. Voce narra o que acontece "
            "em um tom distante, onisciente, levemente divertido. Voce descreve acoes em terceira pessoa "
            "e ocasionalmente se dirige ao jogador diretamente com um toque de passivo-agressividade.\n\n"
            
            "DIRETRIZES DE ESTILO:\n"
            "- Fale em terceira pessoa sobre 'o jogador' ou 'Stanley' (pode usar qualquer um)\n"
            "- Seja articulado, sofisticado, e ocasionalmente sarcastico\n"
            "- Reaja as escolhas do jogador com desapontamento velado ou surpresa\n"
            "- Mantenha respostas focadas na acao atual (1-3 sentencas)\n"
            "- Seu tom deve evoluir baseado no que aconteceu antes\n\n";
    }
    
    if (!event_history.empty()) {
        narrator_prompt += (en ? "WHAT HAPPENED BEFORE:\n" : "O QUE ACONTECEU ANTES:\n") + event_history + "\n";
    }
    
    if (!frustration.empty()) {
        narrator_prompt += frustration + "\n";
    }
    
    narrator_prompt += (en ? "CURRENT EVENT: " : "EVENTO ATUAL: ") + event_desc + "\n\n";
    narrator_prompt += (en ? "YOUR NARRATION:" : "SUA NARRACAO:");
    
    se::AIRequest request;
    request.prompt = narrator_prompt;
    request.max_tokens = config_.max_tokens;
    request.temperature = config_.temperature;
    
    request.on_complete = [this](const std::string& response) {
        if (!messages_.empty() && messages_.back().is_pending) {
            messages_.back().response = response;
            messages_.back().is_pending = false;
            last_narrator_said_ = response;
            
            // Speak the response using TTS
            if (se::TTSManager::Get().IsInitialized()) {
                se::TTSManager::Get().Speak(response);
            }
        }
        waiting_response_ = false;
        status_text_ = language_ == Language::English ? "Narrator Ready" : "Narrador Pronto";
        SE_LOG_INFO("[NarratorLayer] Narrator response: {} chars", response.size());
    };
    
    request.on_error = [this](const std::string& error) {
        if (!messages_.empty() && messages_.back().is_pending) {
            messages_.back().response = "Error: " + error;
            messages_.back().is_pending = false;
        }
        waiting_response_ = false;
        status_text_ = language_ == Language::English ? "Error occurred" : "Erro";
        SE_LOG_ERROR("[NarratorLayer] Error: {}", error);
    };
    
    se::AIManager::Get().QueueRequest(std::move(request));
    SE_LOG_INFO("[NarratorLayer] Event sent: {}", event_desc);
}
