#pragma once

#include <chrono>
#include <string>
#include <unordered_map>
#include <mutex>
#include <atomic>

namespace se {

struct PerformanceMetrics {
    float frameTimeMs = 0.0f;
    float physicsTimeMs = 0.0f;
    float physicsLoopTimeMs = 0.0f;
    float renderTimeMs = 0.0f;
    float updateTimeMs = 0.0f;
    float syncTransformsTimeMs = 0.0f;
    
    // Physics specific
    size_t activePhysicsBodies = 0;
    size_t sleepingPhysicsBodies = 0;
    size_t totalPhysicsBodies = 0;
    bool physicsIdle = false;
    size_t physicsLoopsPerSecond = 0;
    
    // ThreadPool
    size_t threadPoolTasks = 0;
    size_t threadCount = 0;
    
    // Rendering
    uint32_t drawCalls = 0;
    uint32_t triangleCount = 0;
    uint32_t visibleObjects = 0;
    uint32_t totalObjects = 0;
};

class PerformanceProfiler {
public:
    static PerformanceProfiler& Get() {
        static PerformanceProfiler instance;
        return instance;
    }

    void BeginFrame() {
        frame_start_ = std::chrono::high_resolution_clock::now();
        ++frame_count_;
    }

    void EndFrame() {
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::milli> duration = now - frame_start_;
        metrics_.frameTimeMs = duration.count();
        
        // Calculate FPS every second
        auto elapsed = std::chrono::duration<float>(now - fps_timer_).count();
        if (elapsed >= 1.0f) {
            fps_ = static_cast<float>(frame_count_) / elapsed;
            frame_count_ = 0;
            fps_timer_ = now;
        }
    }

    void BeginSection(const std::string& name) {
        section_starts_[name] = std::chrono::high_resolution_clock::now();
    }

    float EndSection(const std::string& name) {
        auto it = section_starts_.find(name);
        if (it == section_starts_.end()) return 0.0f;
        
        auto now = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::milli> duration = now - it->second;
        return duration.count();
    }

    void SetPhysicsMetrics(size_t active, size_t sleeping, size_t total, bool idle, float loopTimeMs) {
        metrics_.activePhysicsBodies = active;
        metrics_.sleepingPhysicsBodies = sleeping;
        metrics_.totalPhysicsBodies = total;
        metrics_.physicsIdle = idle;
        metrics_.physicsLoopTimeMs = loopTimeMs;
    }

    void SetPhysicsTime(float timeMs) { metrics_.physicsTimeMs = timeMs; }
    void SetRenderTime(float timeMs) { metrics_.renderTimeMs = timeMs; }
    void SetUpdateTime(float timeMs) { metrics_.updateTimeMs = timeMs; }
    void SetSyncTransformsTime(float timeMs) { metrics_.syncTransformsTimeMs = timeMs; }
    void SetThreadPoolInfo(size_t taskCount, size_t threadCount) { 
        metrics_.threadPoolTasks = taskCount;
        metrics_.threadCount = threadCount;
    }
    
    void IncrementPhysicsLoop() { ++physics_loops_this_second_; }
    
    void UpdatePhysicsLoopsPerSecond() {
        auto now = std::chrono::high_resolution_clock::now();
        auto elapsed = std::chrono::duration<float>(now - physics_loop_timer_).count();
        if (elapsed >= 1.0f) {
            metrics_.physicsLoopsPerSecond = physics_loops_this_second_.load();
            physics_loops_this_second_ = 0;
            physics_loop_timer_ = now;
        }
    }

    const PerformanceMetrics& GetMetrics() const { return metrics_; }
    float GetFPS() const { return fps_; }

private:
    PerformanceProfiler() = default;
    
    PerformanceMetrics metrics_;
    std::unordered_map<std::string, std::chrono::high_resolution_clock::time_point> section_starts_;
    std::chrono::high_resolution_clock::time_point frame_start_;
    std::chrono::high_resolution_clock::time_point fps_timer_ = std::chrono::high_resolution_clock::now();
    std::chrono::high_resolution_clock::time_point physics_loop_timer_ = std::chrono::high_resolution_clock::now();
    std::atomic<size_t> physics_loops_this_second_{0};
    size_t frame_count_ = 0;
    float fps_ = 0.0f;
};

// RAII helper for timing sections
class ScopedTimer {
public:
    ScopedTimer(const std::string& name, float* outputMs = nullptr)
        : name_(name), output_ms_(outputMs)
        , start_(std::chrono::high_resolution_clock::now()) {}
    
    ~ScopedTimer() {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float, std::milli> duration = end - start_;
        if (output_ms_) {
            *output_ms_ = duration.count();
        }
    }

private:
    std::string name_;
    float* output_ms_;
    std::chrono::high_resolution_clock::time_point start_;
};

#define SE_PROFILE_SCOPE(name) ScopedTimer _timer_##__LINE__(name)
#define SE_PROFILE_SCOPE_MS(name, outputMs) ScopedTimer _timer_##__LINE__(name, outputMs)

} // namespace se
