#include "engine/ai/AIManager.h"
#include "engine/Log.h"

#include "llama.h"
#include "common.h"
#include "sampling.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <atomic>

namespace se {

struct AIManager::Impl {
    AIConfig config;

    llama_model* model   = nullptr;
    llama_context* ctx   = nullptr;
    common_sampler* sampler = nullptr;

    std::thread worker_thread;
    std::atomic<bool> running{false};
    std::atomic<bool> loading{false};
    std::atomic<bool> ready{false};

    std::queue<AIRequest> request_queue;
    std::mutex queue_mutex;
    std::condition_variable queue_cv;

    std::queue<std::function<void()>> callback_queue;
    std::mutex callback_mutex;

    std::atomic<uint32_t> pending_count{0};
    std::atomic<uint32_t> next_id{1};

    void WorkerLoop();
    std::string RunInference(const AIRequest& request);
};

void AIManager::Impl::WorkerLoop() {
    SE_LOG_INFO("[AI] Worker thread started");

    while (running.load()) {
        AIRequest request;

        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            queue_cv.wait(lock, [this] {
                return !request_queue.empty() || !running.load();
            });

            if (!running.load() && request_queue.empty()) {
                break;
            }

            if (request_queue.empty()) {
                continue;
            }

            request = std::move(request_queue.front());
            request_queue.pop();
        }

        SE_LOG_INFO("[AI] Processing request #{}", request.request_id);

        std::string response;
        std::string error;

        try {
            response = RunInference(request);
        } catch (const std::exception& e) {
            error = e.what();
            SE_LOG_ERROR("[AI] Inference error: {}", error);
        }

        {
            std::lock_guard<std::mutex> lock(callback_mutex);
            if (error.empty() && request.on_complete) {
                callback_queue.push([cb = request.on_complete, r = std::move(response)]() {
                    cb(r);
                });
            } else if (!error.empty() && request.on_error) {
                callback_queue.push([cb = request.on_error, e = std::move(error)]() {
                    cb(e);
                });
            }
        }

        pending_count.fetch_sub(1);
        SE_LOG_INFO("[AI] Request #{} completed", request.request_id);
    }

    SE_LOG_INFO("[AI] Worker thread stopped");
}

std::string AIManager::Impl::RunInference(const AIRequest& request) {
    if (!model || !ctx) {
        throw std::runtime_error("Model not loaded");
    }

    const llama_vocab* vocab = llama_model_get_vocab(model);

    std::vector<llama_token> tokens = common_tokenize(ctx, request.prompt, true, true);

    if (tokens.empty()) {
        throw std::runtime_error("Empty prompt after tokenization");
    }

    SE_LOG_DEBUG("[AI] Prompt tokens: {}", tokens.size());

    llama_memory_t mem = llama_get_memory(ctx);
    if (mem) {
        llama_memory_clear(mem, true);
    }

    llama_batch batch = llama_batch_init(static_cast<int>(tokens.size()), 0, 1);

    for (size_t i = 0; i < tokens.size(); ++i) {
        common_batch_add(batch, tokens[i], static_cast<llama_pos>(i), {0}, false);
    }

    if (batch.n_tokens > 0) {
        batch.logits[batch.n_tokens - 1] = 1;
    }

    if (llama_decode(ctx, batch) != 0) {
        llama_batch_free(batch);
        throw std::runtime_error("Failed to decode prompt");
    }

    llama_batch_free(batch);

    if (sampler) {
        common_sampler_free(sampler);
    }

    common_params_sampling sparams;
    sparams.temp = request.temperature;
    sampler = common_sampler_init(model, sparams);

    std::string result;
    int n_cur = static_cast<int>(tokens.size());

    for (int i = 0; i < request.max_tokens; ++i) {
        llama_token new_token = common_sampler_sample(sampler, ctx, -1);
        common_sampler_accept(sampler, new_token, true);

        if (llama_vocab_is_eog(vocab, new_token)) {
            SE_LOG_DEBUG("[AI] End of generation at token {}", i);
            break;
        }

        std::string piece = common_token_to_piece(ctx, new_token, false);
        result += piece;

        llama_batch single = llama_batch_init(1, 0, 1);
        common_batch_add(single, new_token, n_cur, {0}, true);

        if (llama_decode(ctx, single) != 0) {
            llama_batch_free(single);
            SE_LOG_WARN("[AI] Decode failed at token {}", i);
            break;
        }

        llama_batch_free(single);
        n_cur++;
    }

    SE_LOG_DEBUG("[AI] Generated {} characters", result.size());
    return result;
}

AIManager& AIManager::Get() {
    static AIManager instance;
    return instance;
}

AIManager::AIManager() : m_impl(std::make_unique<Impl>()) {}

AIManager::~AIManager() {
    Shutdown();
}

bool AIManager::Initialize(const AIConfig& config) {
    if (m_impl->ready.load() || m_impl->loading.load()) {
        SE_LOG_WARN("[AI] Already initialized or loading");
        return false;
    }

    m_impl->config = config;
    m_impl->loading.store(true);

    SE_LOG_INFO("[AI] Initializing llama.cpp backend...");
    llama_backend_init();

    SE_LOG_INFO("[AI] Loading model from HF: {} / {}", config.hf_repo, 
        config.hf_file.empty() ? "auto" : config.hf_file);

    common_params params;
    params.model.hf_repo = config.hf_repo;
    params.model.hf_file = config.hf_file;
    params.n_ctx         = config.n_ctx;
    params.cpuparams.n_threads = config.n_threads;
    params.n_gpu_layers  = config.n_gpu_layers;

    common_init_result init_result = common_init_from_params(params);

    if (!init_result.model || !init_result.context) {
        SE_LOG_ERROR("[AI] Failed to load model");
        m_impl->loading.store(false);
        return false;
    }

    m_impl->model = init_result.model.release();
    m_impl->ctx   = init_result.context.release();

    SE_LOG_INFO("[AI] Model loaded successfully");

    m_impl->running.store(true);
    m_impl->worker_thread = std::thread(&Impl::WorkerLoop, m_impl.get());

    m_impl->loading.store(false);
    m_impl->ready.store(true);

    SE_LOG_INFO("[AI] AIManager initialized");
    return true;
}

void AIManager::Shutdown() {
    if (!m_impl->ready.load() && !m_impl->running.load()) {
        return;
    }

    SE_LOG_INFO("[AI] Shutting down...");

    m_impl->running.store(false);
    m_impl->queue_cv.notify_all();

    if (m_impl->worker_thread.joinable()) {
        m_impl->worker_thread.join();
    }

    if (m_impl->sampler) {
        common_sampler_free(m_impl->sampler);
        m_impl->sampler = nullptr;
    }

    if (m_impl->ctx) {
        llama_free(m_impl->ctx);
        m_impl->ctx = nullptr;
    }

    if (m_impl->model) {
        llama_model_free(m_impl->model);
        m_impl->model = nullptr;
    }

    llama_backend_free();

    m_impl->ready.store(false);
    SE_LOG_INFO("[AI] AIManager shutdown complete");
}

bool AIManager::IsReady() const {
    return m_impl->ready.load();
}

bool AIManager::IsLoading() const {
    return m_impl->loading.load();
}

void AIManager::QueueRequest(AIRequest request) {
    if (!m_impl->ready.load()) {
        SE_LOG_ERROR("[AI] Cannot queue request - not initialized");
        if (request.on_error) {
            request.on_error("AI not initialized");
        }
        return;
    }

    request.request_id = m_impl->next_id.fetch_add(1);
    m_impl->pending_count.fetch_add(1);

    {
        std::lock_guard<std::mutex> lock(m_impl->queue_mutex);
        m_impl->request_queue.push(std::move(request));
    }

    m_impl->queue_cv.notify_one();
    SE_LOG_DEBUG("[AI] Request #{} queued", request.request_id);
}

void AIManager::ProcessCallbacks() {
    std::queue<std::function<void()>> callbacks;

    {
        std::lock_guard<std::mutex> lock(m_impl->callback_mutex);
        std::swap(callbacks, m_impl->callback_queue);
    }

    while (!callbacks.empty()) {
        callbacks.front()();
        callbacks.pop();
    }
}

uint32_t AIManager::GetPendingRequestCount() const {
    return m_impl->pending_count.load();
}

} // namespace se
