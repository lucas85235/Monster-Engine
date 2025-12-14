#pragma once

#include "engine/Layer.h"
#include "engine/ecs/Scene.h"
#include <string>
#include <vector>
#include <deque>

using namespace se;

// Configuration for AI Chat - modify these values easily
struct AIChatConfig {
    // Model settings (download larger model for better quality)
    // Qwen3-4B: https://huggingface.co/Qwen/Qwen3-4B-GGUF/resolve/main/qwen3-4b-q4_k_m.gguf
    std::string model_file = "qwen3-4b-q4_k_m.gguf";
    
    // Performance settings
    int context_size = 4096;     // Context window size (larger = better memory)
    int max_threads = 14;        // Number of CPU threads to use
    int gpu_layers = 40;         // Number of layers to offload to GPU (0 = CPU only)
    
    // Generation settings (Qwen3 Non-Thinking Mode recommended values)
    int max_tokens = 512;        // Maximum tokens to generate per response
    float temperature = 0.7f;    // Recommended for non-thinking mode
    float top_p = 0.8f;          // Nucleus sampling threshold
    int top_k = 20;              // Top-K sampling
    float min_p = 0.0f;          // Min-P sampling (0 = disabled)
    
    // Anti-repetition settings
    float penalty_repeat = 1.05f;   // Repetition penalty (1.0 = disabled)
    int penalty_last_n = 64;        // Tokens to look back for repetition
    float dry_multiplier = 0.0f;    // DRY penalty (disabled for Qwen3)
};

struct ChatMessage {
    std::string role;
    std::string content;
    bool is_pending = false;
};

class AIChatLayer : public se::Layer {
public:
    AIChatLayer();
    ~AIChatLayer() override;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float ts) override;
    void OnRender() override;
    void OnImGuiRender() override;

private:
    void SendMessage();
    void RenderChatWindow();
    void RenderStatusBar();

private:
    Scope<Scene> scene_;
    AIChatConfig config_;  // AI configuration - modify defaults in struct above
    
    char input_buffer_[1024] = {0};
    std::deque<ChatMessage> messages_;
    
    bool ai_initialized_ = false;
    bool ai_loading_ = false;
    bool waiting_response_ = false;
    bool auto_scroll_ = true;
    
    std::string status_text_ = "Not initialized";
};
