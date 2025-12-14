#pragma once

#include "engine/Layer.h"
#include "engine/ecs/Scene.h"
#include "engine/ecs/Entity.h"
#include "engine/Camera.h"
#include "engine/renderer/CameraController.h"
#include "engine/renderer/Material.h"
#include <string>
#include <vector>
#include <deque>
#include <unordered_map>
#include <unordered_set>

using namespace se;

// Narrator configuration
struct NarratorConfig {
    // Model settings
    std::string model_file = "qwen3-4b-q4_k_m.gguf";
    
    // Performance
    int context_size = 32000;
    int max_threads = 14;
    int gpu_layers = 40;
    
    // Generation (Qwen3 recommended)
    int max_tokens = 256;
    float temperature = 0.7f;
    float top_p = 0.85f;
    int top_k = 30;
    float min_p = 0.0f;
    float penalty_repeat = 1.5f;
    int penalty_last_n = 128;
    float dry_multiplier = 0.8f;
};

enum class Language {
    English,
    Portuguese
};

enum class GameEvent {
    // Movement triggers
    PlayerStartedWalking,
    PlayerReachedBifurcation,
    PlayerWentLeft,
    PlayerWentRight,
    PlayerEnteredLeftRoom,
    PlayerEnteredRightRoom,
    PlayerReturnedToStart,
    
    // Interactive
    PlayerOpenedDoor,
    PlayerFoundSecret,
    
    // Narrator commands
    NarratorSaidGoLeft,
    NarratorSaidGoRight,
    
    // Player responses
    PlayerObeyedCommand,
    PlayerDisobeyedCommand,
    
    Custom
};

struct NarratorMessage {
    std::string event_name;
    std::string response;
    int repetition_count = 0;
    bool is_pending = false;
};

struct TriggerZone {
    std::string name;
    GameEvent event;
    glm::vec3 position;
    glm::vec3 size;
    bool triggered = false;
    bool one_shot = true;  // Only trigger once
};

class NarratorLayer : public se::Layer {
public:
    NarratorLayer();
    ~NarratorLayer() override;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float ts) override;
    void OnRender() override;
    void OnImGuiRender() override;
    void OnEvent(Event& event) override;

private:
    // Setup
    void SetupScene();
    void SetupTriggers();
    void SetupInputBindings();
    
    // Gameplay
    void UpdatePlayer(float ts);
    void CheckTriggers();
    
    // AI
    void SendEvent(GameEvent event, const std::string& custom_event = "");
    std::string GetEventDescription(GameEvent event);
    std::string GetFrustrationLevel(int count);
    
    // UI
    void RenderGameUI();
    void RenderNarratorWindow();
    void RenderDebugWindow();
    void RenderLanguageSelector();

private:
    // Scene
    Scope<Scene> scene_;
    std::shared_ptr<Material> material_;
    
    // Camera and player
    Camera camera_;
    Scope<CameraController> camera_controller_;
    Entity player_entity_;
    glm::vec3 player_velocity_{0.0f};
    bool is_grounded_ = true;
    
    // Triggers
    std::vector<TriggerZone> triggers_;
    std::unordered_set<std::string> triggered_zones_;
    
    // Narrator
    NarratorConfig config_;
    std::deque<NarratorMessage> messages_;
    std::unordered_map<int, int> event_counts_;
    char custom_event_buffer_[512] = {0};
    
    // State
    Language language_ = Language::English;
    bool ai_initialized_ = false;
    bool ai_loading_ = false;
    bool waiting_response_ = false;
    bool game_mode_ = false;  // Toggle between game mode and UI mode
    std::string status_text_ = "Not initialized";
    std::string last_narrator_said_;
};
