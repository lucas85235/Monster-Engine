#include "AIChatLayer.h"
#include "engine/ai/AIManager.h"
#include "engine/Log.h"

#include <imgui.h>

AIChatLayer::AIChatLayer() : Layer("AIChatLayer") {
    SE_LOG_INFO("[AIChatLayer] Constructed");
}

AIChatLayer::~AIChatLayer() {
    SE_LOG_INFO("[AIChatLayer] Destroyed");
}

void AIChatLayer::OnAttach() {
    SE_LOG_INFO("[AIChatLayer] OnAttach - Initializing AI...");
    
    scene_ = CreateScope<Scene>();
    
    messages_.push_back({"system", "AI Chat Demo - Type a message and press Enter to chat with the AI.", false});
    
    // Build model path from config
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
    status_text_ = "Loading AI model...";
    
    std::thread init_thread([this, ai_config]() {
        bool success = se::AIManager::Get().Initialize(ai_config);
        ai_loading_ = false;
        if (success) {
            ai_initialized_ = true;
            status_text_ = "AI Ready";
            SE_LOG_INFO("[AIChatLayer] AI initialized successfully");
        } else {
            status_text_ = "Failed to initialize AI";
            SE_LOG_ERROR("[AIChatLayer] Failed to initialize AI");
        }
    });
    init_thread.detach();
}

void AIChatLayer::OnDetach() {
    SE_LOG_INFO("[AIChatLayer] OnDetach - Shutting down AI...");
    se::AIManager::Get().Shutdown();
}

void AIChatLayer::OnUpdate(float ts) {
    se::AIManager::Get().ProcessCallbacks();
}

void AIChatLayer::OnRender() {
}

void AIChatLayer::OnImGuiRender() {
    RenderChatWindow();
    RenderStatusBar();
}

void AIChatLayer::RenderChatWindow() {
    ImGui::SetNextWindowSize(ImVec2(600, 500), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(50, 50), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("AI Chat", nullptr, ImGuiWindowFlags_NoCollapse);
    
    float footer_height = ImGui::GetStyle().ItemSpacing.y + ImGui::GetFrameHeightWithSpacing();
    ImGui::BeginChild("ChatMessages", ImVec2(0, -footer_height), true, ImGuiWindowFlags_HorizontalScrollbar);
    
    for (const auto& msg : messages_) {
        if (msg.role == "system") {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.6f, 0.6f, 1.0f));
            ImGui::TextWrapped("[System] %s", msg.content.c_str());
            ImGui::PopStyleColor();
        } else if (msg.role == "user") {
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.8f, 1.0f, 1.0f));
            ImGui::TextWrapped("[You] %s", msg.content.c_str());
            ImGui::PopStyleColor();
        } else if (msg.role == "assistant") {
            if (msg.is_pending) {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
                ImGui::TextWrapped("[AI] Thinking...");
                ImGui::PopStyleColor();
            } else {
                ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 1.0f, 0.4f, 1.0f));
                ImGui::TextWrapped("[AI] %s", msg.content.c_str());
                ImGui::PopStyleColor();
            }
        }
        ImGui::Spacing();
    }
    
    if (auto_scroll_ && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()) {
        ImGui::SetScrollHereY(1.0f);
    }
    
    ImGui::EndChild();
    
    ImGui::Separator();
    
    bool send_pressed = false;
    ImGui::PushItemWidth(-80);
    if (ImGui::InputText("##ChatInput", input_buffer_, sizeof(input_buffer_), 
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
        send_pressed = true;
    }
    ImGui::PopItemWidth();
    
    ImGui::SameLine();
    
    bool can_send = ai_initialized_ && !waiting_response_ && strlen(input_buffer_) > 0;
    
    // Gray out button if can't send
    if (!can_send) {
        ImGui::PushStyleVar(ImGuiStyleVar_Alpha, 0.5f);
    }
    if (ImGui::Button("Send", ImVec2(70, 0)) && can_send) {
        SendMessage();
    }
    if (!can_send) {
        ImGui::PopStyleVar();
    }
    
    ImGui::End();
}

void AIChatLayer::RenderStatusBar() {
    ImGui::SetNextWindowSize(ImVec2(300, 60), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowPos(ImVec2(660, 50), ImGuiCond_FirstUseEver);
    
    ImGui::Begin("AI Status", nullptr, ImGuiWindowFlags_NoCollapse);
    
    if (ai_loading_) {
        ImGui::TextColored(ImVec4(1.0f, 1.0f, 0.0f, 1.0f), "Loading...");
    } else if (ai_initialized_) {
        ImGui::TextColored(ImVec4(0.0f, 1.0f, 0.0f, 1.0f), "Ready");
    } else {
        ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "Not Ready");
    }
    
    ImGui::SameLine();
    ImGui::Text("| %s", status_text_.c_str());
    
    if (waiting_response_) {
        ImGui::Text("Processing request...");
    }
    
    ImGui::End();
}

void AIChatLayer::SendMessage() {
    std::string user_input = input_buffer_;
    if (user_input.empty()) return;
    
    messages_.push_back({"user", user_input, false});
    messages_.push_back({"assistant", "", true});
    
    memset(input_buffer_, 0, sizeof(input_buffer_));
    waiting_response_ = true;
    status_text_ = "Generating response...";
    
    se::AIRequest request;
    request.prompt = user_input;
    request.max_tokens = config_.max_tokens;
    request.temperature = config_.temperature;
    
    request.on_complete = [this](const std::string& response) {
        if (!messages_.empty() && messages_.back().is_pending) {
            messages_.back().content = response;
            messages_.back().is_pending = false;
        }
        waiting_response_ = false;
        status_text_ = "AI Ready";
        SE_LOG_INFO("[AIChatLayer] Received response: {} chars", response.size());
    };
    
    request.on_error = [this](const std::string& error) {
        if (!messages_.empty() && messages_.back().is_pending) {
            messages_.back().content = "Error: " + error;
            messages_.back().is_pending = false;
        }
        waiting_response_ = false;
        status_text_ = "Error occurred";
        SE_LOG_ERROR("[AIChatLayer] AI error: {}", error);
    };
    
    se::AIManager::Get().QueueRequest(std::move(request));
    SE_LOG_INFO("[AIChatLayer] Sent request to AI");
}
