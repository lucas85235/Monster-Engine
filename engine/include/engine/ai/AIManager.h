#pragma once

#include <string>
#include <memory>
#include <functional>
#include <cstdint>

namespace se {

struct AIConfig {
    std::string model_path = "";  // Local path to model file (takes priority)
    std::string hf_repo    = "";  // HuggingFace repo (fallback)
    std::string hf_file    = "";  // HuggingFace file
    int         n_ctx      = 4096;
    int         n_threads  = 4;
    int         n_gpu_layers = 0;
    
    // Sampling parameters
    float       temperature    = 0.7f;
    float       top_p          = 0.8f;
    int         top_k          = 20;
    float       min_p          = 0.0f;
    float       penalty_repeat = 1.05f;
    int         penalty_last_n = 64;
    float       dry_multiplier = 0.0f;
};

struct AIRequest {
    std::string prompt;
    int         max_tokens   = 256;
    float       temperature  = 0.7f;
    uint32_t    request_id   = 0;

    std::function<void(const std::string&)> on_complete;
    std::function<void(const std::string&)> on_error;
};

class AIManager {
public:
    static AIManager& Get();

    bool Initialize(const AIConfig& config = {});
    void Shutdown();
    bool IsReady() const;
    bool IsLoading() const;

    void QueueRequest(AIRequest request);
    void ProcessCallbacks();

    uint32_t GetPendingRequestCount() const;

private:
    AIManager();
    ~AIManager();

    AIManager(const AIManager&) = delete;
    AIManager& operator=(const AIManager&) = delete;

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace se
