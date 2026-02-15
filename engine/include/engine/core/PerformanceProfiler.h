#pragma once

#include <atomic>
#include <chrono>
#include <mutex>
#include <string>
#include <string_view>
#include <unordered_map>

namespace se {

struct PerformanceMetrics {
    float frameTimeMs          = 0.0f;
    float physicsTimeMs        = 0.0f;
    float physicsLoopTimeMs    = 0.0f;
    float renderTimeMs         = 0.0f;
    float updateTimeMs         = 0.0f;
    float syncTransformsTimeMs = 0.0f;

    // Main-loop CPU breakdown
    float inputUpdateTimeMs    = 0.0f;
    float eventPumpTimeMs      = 0.0f;
    float settingsApplyTimeMs  = 0.0f;
    float resizeHandlingTimeMs = 0.0f;
    float renderSetupTimeMs    = 0.0f;
    float layerUpdateTimeMs    = 0.0f;
    float animationTimeMs      = 0.0f;
    float layerRenderTimeMs    = 0.0f;
    float uiEndFrameTimeMs     = 0.0f;
    float presentTimeMs        = 0.0f;
    float frameLimiterTimeMs   = 0.0f;

    // Physics specific
    size_t activePhysicsBodies   = 0;
    size_t sleepingPhysicsBodies = 0;
    size_t totalPhysicsBodies    = 0;
    bool   physicsIdle           = false;
    size_t physicsLoopsPerSecond = 0;

    // ThreadPool
    size_t threadPoolTasks = 0;
    size_t threadCount     = 0;

    // Rendering
    uint32_t drawCalls      = 0;
    uint32_t triangleCount  = 0;
    uint32_t visibleObjects = 0;
    uint32_t totalObjects   = 0;
};

class PerformanceProfiler {
   public:
    struct FrameSectionTimes {
        float inputUpdateTimeMs    = 0.0f;
        float eventPumpTimeMs      = 0.0f;
        float settingsApplyTimeMs  = 0.0f;
        float resizeHandlingTimeMs = 0.0f;
        float renderSetupTimeMs    = 0.0f;
        float layerUpdateTimeMs    = 0.0f;
        float animationTimeMs      = 0.0f;
        float layerRenderTimeMs    = 0.0f;
        float uiEndFrameTimeMs     = 0.0f;
        float presentTimeMs        = 0.0f;
        float frameLimiterTimeMs   = 0.0f;
    };

    static PerformanceProfiler& Get() {
        static PerformanceProfiler instance;
        return instance;
    }

    void BeginFrame() {
        frame_start_ = std::chrono::high_resolution_clock::now();
        ++frame_count_;
        ResetFrameSectionMetrics();
    }

    void EndFrame() {
        auto                                     now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::milli> duration = now - frame_start_;
        metrics_.frameTimeMs                              = duration.count();
        UpdateAverageMetrics();

        // Calculate FPS every second
        auto elapsed = std::chrono::duration<float>(now - fps_timer_).count();
        if (elapsed >= 1.0f) {
            fps_         = static_cast<float>(frame_count_) / elapsed;
            frame_count_ = 0;
            fps_timer_   = now;
        }
    }

    void BeginSection(std::string_view name) {
        section_starts_[std::string(name)] = std::chrono::high_resolution_clock::now();
    }

    float EndSection(std::string_view name) {
        auto it = section_starts_.find(std::string(name));
        if (it == section_starts_.end()) return 0.0f;

        auto                                     now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::milli> duration = now - it->second;
        return duration.count();
    }

    void SetPhysicsMetrics(size_t active, size_t sleeping, size_t total, bool idle,
                           float loopTimeMs) {
        metrics_.activePhysicsBodies   = active;
        metrics_.sleepingPhysicsBodies = sleeping;
        metrics_.totalPhysicsBodies    = total;
        metrics_.physicsIdle           = idle;
        metrics_.physicsLoopTimeMs     = loopTimeMs;
    }

    void SetPhysicsTime(float timeMs) {
        metrics_.physicsTimeMs = timeMs;
    }
    void SetRenderTime(float timeMs) {
        metrics_.renderTimeMs = timeMs;
    }
    void SetUpdateTime(float timeMs) {
        metrics_.updateTimeMs = timeMs;
    }
    void SetSyncTransformsTime(float timeMs) {
        metrics_.syncTransformsTimeMs = timeMs;
    }
    void SetFrameSectionTimes(const FrameSectionTimes& sections) {
        metrics_.inputUpdateTimeMs    = sections.inputUpdateTimeMs;
        metrics_.eventPumpTimeMs      = sections.eventPumpTimeMs;
        metrics_.settingsApplyTimeMs  = sections.settingsApplyTimeMs;
        metrics_.resizeHandlingTimeMs = sections.resizeHandlingTimeMs;
        metrics_.renderSetupTimeMs    = sections.renderSetupTimeMs;
        metrics_.layerUpdateTimeMs    = sections.layerUpdateTimeMs;
        metrics_.animationTimeMs      = sections.animationTimeMs;
        metrics_.layerRenderTimeMs    = sections.layerRenderTimeMs;
        metrics_.uiEndFrameTimeMs     = sections.uiEndFrameTimeMs;
        metrics_.presentTimeMs        = sections.presentTimeMs;
        metrics_.frameLimiterTimeMs   = sections.frameLimiterTimeMs;
    }
    void SetThreadPoolInfo(size_t taskCount, size_t threadCount) {
        metrics_.threadPoolTasks = taskCount;
        metrics_.threadCount     = threadCount;
    }

    void IncrementPhysicsLoop() {
        ++physics_loops_this_second_;
    }

    void UpdatePhysicsLoopsPerSecond() {
        auto now     = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration<float>(now - physics_loop_timer_).count();
        if (elapsed >= 1.0f) {
            metrics_.physicsLoopsPerSecond = physics_loops_this_second_.load();
            physics_loops_this_second_     = 0;
            physics_loop_timer_            = now;
        }
    }

    const PerformanceMetrics& GetMetrics() const {
        return metrics_;
    }
    const PerformanceMetrics& GetAveragedMetrics() const {
        return avg_metrics_;
    }
    float GetFPS() const {
        return fps_;
    }

    void ResetStats() {
        metrics_      = PerformanceMetrics{};
        avg_metrics_  = PerformanceMetrics{};
        frame_count_  = 0;
        fps_          = 0.0f;
        fps_timer_    = std::chrono::high_resolution_clock::now();
        physics_loop_timer_ = fps_timer_;
        physics_loops_this_second_ = 0;
    }

   private:
    PerformanceProfiler() = default;

    void ResetFrameSectionMetrics() {
        metrics_.inputUpdateTimeMs    = 0.0f;
        metrics_.eventPumpTimeMs      = 0.0f;
        metrics_.settingsApplyTimeMs  = 0.0f;
        metrics_.resizeHandlingTimeMs = 0.0f;
        metrics_.renderSetupTimeMs    = 0.0f;
        metrics_.layerUpdateTimeMs    = 0.0f;
        metrics_.animationTimeMs      = 0.0f;
        metrics_.layerRenderTimeMs    = 0.0f;
        metrics_.uiEndFrameTimeMs     = 0.0f;
        metrics_.presentTimeMs        = 0.0f;
        metrics_.frameLimiterTimeMs   = 0.0f;
        metrics_.updateTimeMs         = 0.0f;
        metrics_.renderTimeMs         = 0.0f;
    }

    static float Lerp(float a, float b, float t) {
        return a + (b - a) * t;
    }

    void UpdateAverageMetrics() {
        const float alpha = 0.12f;
        auto blend = [alpha](float current, float sample) { return Lerp(current, sample, alpha); };

        avg_metrics_.frameTimeMs          = blend(avg_metrics_.frameTimeMs, metrics_.frameTimeMs);
        avg_metrics_.physicsTimeMs        = blend(avg_metrics_.physicsTimeMs, metrics_.physicsTimeMs);
        avg_metrics_.physicsLoopTimeMs    = blend(avg_metrics_.physicsLoopTimeMs, metrics_.physicsLoopTimeMs);
        avg_metrics_.renderTimeMs         = blend(avg_metrics_.renderTimeMs, metrics_.renderTimeMs);
        avg_metrics_.updateTimeMs         = blend(avg_metrics_.updateTimeMs, metrics_.updateTimeMs);
        avg_metrics_.syncTransformsTimeMs = blend(avg_metrics_.syncTransformsTimeMs, metrics_.syncTransformsTimeMs);

        avg_metrics_.inputUpdateTimeMs    = blend(avg_metrics_.inputUpdateTimeMs, metrics_.inputUpdateTimeMs);
        avg_metrics_.eventPumpTimeMs      = blend(avg_metrics_.eventPumpTimeMs, metrics_.eventPumpTimeMs);
        avg_metrics_.settingsApplyTimeMs  = blend(avg_metrics_.settingsApplyTimeMs, metrics_.settingsApplyTimeMs);
        avg_metrics_.resizeHandlingTimeMs = blend(avg_metrics_.resizeHandlingTimeMs, metrics_.resizeHandlingTimeMs);
        avg_metrics_.renderSetupTimeMs    = blend(avg_metrics_.renderSetupTimeMs, metrics_.renderSetupTimeMs);
        avg_metrics_.layerUpdateTimeMs    = blend(avg_metrics_.layerUpdateTimeMs, metrics_.layerUpdateTimeMs);
        avg_metrics_.animationTimeMs      = blend(avg_metrics_.animationTimeMs, metrics_.animationTimeMs);
        avg_metrics_.layerRenderTimeMs    = blend(avg_metrics_.layerRenderTimeMs, metrics_.layerRenderTimeMs);
        avg_metrics_.uiEndFrameTimeMs     = blend(avg_metrics_.uiEndFrameTimeMs, metrics_.uiEndFrameTimeMs);
        avg_metrics_.presentTimeMs        = blend(avg_metrics_.presentTimeMs, metrics_.presentTimeMs);
        avg_metrics_.frameLimiterTimeMs   = blend(avg_metrics_.frameLimiterTimeMs, metrics_.frameLimiterTimeMs);

        avg_metrics_.activePhysicsBodies   = metrics_.activePhysicsBodies;
        avg_metrics_.sleepingPhysicsBodies = metrics_.sleepingPhysicsBodies;
        avg_metrics_.totalPhysicsBodies    = metrics_.totalPhysicsBodies;
        avg_metrics_.physicsIdle           = metrics_.physicsIdle;
        avg_metrics_.physicsLoopsPerSecond = metrics_.physicsLoopsPerSecond;
        avg_metrics_.threadPoolTasks       = metrics_.threadPoolTasks;
        avg_metrics_.threadCount           = metrics_.threadCount;
        avg_metrics_.drawCalls             = metrics_.drawCalls;
        avg_metrics_.triangleCount         = metrics_.triangleCount;
        avg_metrics_.visibleObjects        = metrics_.visibleObjects;
        avg_metrics_.totalObjects          = metrics_.totalObjects;
    }

    PerformanceMetrics                                                              metrics_;
    PerformanceMetrics                                                              avg_metrics_;
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> section_starts_;
    std::chrono::high_resolution_clock::time_point                                  frame_start_;
    std::chrono::high_resolution_clock::time_point                                  fps_timer_ =
        std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::time_point physics_loop_timer_ =
        std::chrono::high_resolution_clock::now();
    std::atomic<size_t> physics_loops_this_second_{0};
    size_t              frame_count_ = 0;
    float               fps_         = 0.0f;
};

// RAII helper for timing sections (uses const char* to avoid string allocation)
class ScopedTimer {
   public:
    ScopedTimer(const char* name, float* outputMs = nullptr)
        : name_(name), output_ms_(outputMs), start_(std::chrono::high_resolution_clock::now()) {}

    ~ScopedTimer() {
        auto                                     end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::milli> duration = end - start_;
        if (output_ms_) { *output_ms_ = duration.count(); }
    }

   private:
    const char*                                    name_;
    float*                                         output_ms_;
    std::chrono::high_resolution_clock::time_point start_;
};

#define SE_PROFILE_SCOPE(name)              ScopedTimer _timer_##__LINE__(name)
#define SE_PROFILE_SCOPE_MS(name, outputMs) ScopedTimer _timer_##__LINE__(name, outputMs)

}  // namespace se
