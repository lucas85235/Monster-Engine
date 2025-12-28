#pragma once

#include "DebugConfig.h"

#if SE_ENABLE_PROFILER

#include "IDebugTool.h"

#include <vector>
#include <string>
#include <chrono>

// Forward declarations to avoid including heavy headers everywhere
namespace legit {
    struct ProfilerTask;
}

namespace ImGuiUtils {
    class ProfilersWindow;
}

namespace se::debug {

/**
 * Frame Profiler - CPU/GPU timing visualization using Legit Profiler.
 * 
 * Usage:
 *   SE_PROFILE_FUNCTION();           // Profile entire function
 *   SE_PROFILE_SCOPE("Loading");     // Profile specific scope
 */
class FrameProfiler : public IDebugTool {
public:
    static FrameProfiler& Get();
    
    const char* GetName() const override { return "Frame Profiler"; }
    void OnImGuiRender() override;
    void OnFrameStart() override;
    void OnFrameEnd() override;
    bool IsEnabledByDefault() const override { return true; }
    
    // Begin a profiled task (CPU)
    void BeginCpuTask(const char* name, uint32_t color);
    void EndCpuTask();
    
    // Begin a profiled task (GPU)
    void BeginGpuTask(const char* name, uint32_t color);
    void EndGpuTask();
    
    // Get current frame time
    double GetCurrentTime() const;
    
private:
    FrameProfiler();
    ~FrameProfiler();
    
    // Prevent copies
    FrameProfiler(const FrameProfiler&) = delete;
    FrameProfiler& operator=(const FrameProfiler&) = delete;
    
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * RAII helper for scoped profiling.
 */
class ScopedCpuTask {
public:
    ScopedCpuTask(const char* name, uint32_t color);
    ~ScopedCpuTask();
    
private:
    ScopedCpuTask(const ScopedCpuTask&) = delete;
    ScopedCpuTask& operator=(const ScopedCpuTask&) = delete;
};

class ScopedGpuTask {
public:
    ScopedGpuTask(const char* name, uint32_t color);
    ~ScopedGpuTask();
    
private:
    ScopedGpuTask(const ScopedGpuTask&) = delete;
    ScopedGpuTask& operator=(const ScopedGpuTask&) = delete;
};

} // namespace se::debug

// Profiler color palette (from Legit Profiler)
namespace ProfilerColors {
    constexpr uint32_t Turquoise   = 0xFF9CBC1A;
    constexpr uint32_t Emerald     = 0xFF71CC2E;
    constexpr uint32_t PeterRiver  = 0xFFDB9834; // Blue
    constexpr uint32_t Amethyst    = 0xFFB6599B;
    constexpr uint32_t SunFlower   = 0xFF0FC4F1;
    constexpr uint32_t Carrot      = 0xFF227EE6;
    constexpr uint32_t Alizarin    = 0xFF3C4CE7; // Red
    constexpr uint32_t Clouds      = 0xFFF1F0EC;
}

// Macro magic for unique variable names
#define SE_PROFILER_CONCAT_IMPL(a, b) a##b
#define SE_PROFILER_CONCAT(a, b) SE_PROFILER_CONCAT_IMPL(a, b)

// User-facing macros
#define SE_PROFILE_FUNCTION() \
    ::se::debug::ScopedCpuTask SE_PROFILER_CONCAT(_profiler_, __LINE__)(__FUNCTION__, ProfilerColors::PeterRiver)

#define SE_PROFILE_SCOPE(name) \
    ::se::debug::ScopedCpuTask SE_PROFILER_CONCAT(_profiler_, __LINE__)(name, ProfilerColors::Emerald)

#define SE_PROFILE_SCOPE_COLOR(name, color) \
    ::se::debug::ScopedCpuTask SE_PROFILER_CONCAT(_profiler_, __LINE__)(name, color)

#define SE_PROFILE_GPU(name) \
    ::se::debug::ScopedGpuTask SE_PROFILER_CONCAT(_profiler_gpu_, __LINE__)(name, ProfilerColors::Amethyst)

#else // SE_ENABLE_PROFILER

// Release build - all macros expand to nothing (zero overhead)
#define SE_PROFILE_FUNCTION()
#define SE_PROFILE_SCOPE(name)
#define SE_PROFILE_SCOPE_COLOR(name, color)
#define SE_PROFILE_GPU(name)

#endif // SE_ENABLE_PROFILER
