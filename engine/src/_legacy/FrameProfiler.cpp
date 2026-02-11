#include "engine/debug/FrameProfiler.h"

#if SE_ENABLE_PROFILER

#include <glm.hpp>
#include "ProfilerTask.h"
#include "ImGuiProfilerRenderer.h"

#include <chrono>
#include <stack>

namespace se::debug {

// Pimpl to hide Legit Profiler implementation details
struct FrameProfiler::Impl {
    ImGuiUtils::ProfilersWindow profilerWindow{1.0f / 60.0f};
    std::vector<legit::ProfilerTask> cpuTasks;
    std::vector<legit::ProfilerTask> gpuTasks;
    std::stack<size_t> cpuTaskStack;
    std::stack<size_t> gpuTaskStack;
    
    using Clock = std::chrono::high_resolution_clock;
    Clock::time_point frameStartTime;
    
    Impl() {
        cpuTasks.reserve(SE_PROFILER_MAX_TASKS);
        gpuTasks.reserve(SE_PROFILER_MAX_TASKS);
    }
    
    double GetElapsedSeconds() const {
        auto now = Clock::now();
        return std::chrono::duration<double>(now - frameStartTime).count();
    }
};

FrameProfiler& FrameProfiler::Get() {
    static FrameProfiler instance;
    return instance;
}

FrameProfiler::FrameProfiler() : impl_(std::make_unique<Impl>()) {}

FrameProfiler::~FrameProfiler() = default;

void FrameProfiler::OnFrameStart() {
    impl_->frameStartTime = Impl::Clock::now();
    impl_->cpuTasks.clear();
    impl_->gpuTasks.clear();
    
    // Clear stacks in case of unbalanced Begin/End
    while (!impl_->cpuTaskStack.empty()) impl_->cpuTaskStack.pop();
    while (!impl_->gpuTaskStack.empty()) impl_->gpuTaskStack.pop();
}

void FrameProfiler::OnFrameEnd() {
    // Load frame data into the profiler graphs
    if (!impl_->profilerWindow.stopProfiling) {
        impl_->profilerWindow.cpuGraph.LoadFrameData(
            impl_->cpuTasks.data(), 
            impl_->cpuTasks.size()
        );
        impl_->profilerWindow.gpuGraph.LoadFrameData(
            impl_->gpuTasks.data(), 
            impl_->gpuTasks.size()
        );
    }
}

void FrameProfiler::OnImGuiRender() {
    impl_->profilerWindow.Render();
}

double FrameProfiler::GetCurrentTime() const {
    return impl_->GetElapsedSeconds();
}

void FrameProfiler::BeginCpuTask(const char* name, uint32_t color) {
    legit::ProfilerTask task;
    task.startTime = impl_->GetElapsedSeconds();
    task.endTime = task.startTime;
    task.name = name;
    
    // Generate unique color from name hash if using default color
    if (color == ProfilerColors::Emerald || color == ProfilerColors::PeterRiver) {
        // Hash the name to get a consistent color per task name
        size_t hash = std::hash<std::string>{}(name);
        
        // Generate vibrant HSV color and convert to RGBA
        float hue = (hash % 360) / 360.0f;
        float saturation = 0.7f + (((hash >> 8) % 30) / 100.0f);  // 0.7-1.0
        float value = 0.8f + (((hash >> 16) % 20) / 100.0f);      // 0.8-1.0
        
        // HSV to RGB conversion
        int hi = static_cast<int>(hue * 6.0f) % 6;
        float f = hue * 6.0f - hi;
        float p = value * (1.0f - saturation);
        float q = value * (1.0f - f * saturation);
        float t = value * (1.0f - (1.0f - f) * saturation);
        
        float r, g, b;
        switch (hi) {
            case 0: r = value; g = t; b = p; break;
            case 1: r = q; g = value; b = p; break;
            case 2: r = p; g = value; b = t; break;
            case 3: r = p; g = q; b = value; break;
            case 4: r = t; g = p; b = value; break;
            default: r = value; g = p; b = q; break;
        }
        
        uint8_t rr = static_cast<uint8_t>(r * 255.0f);
        uint8_t gg = static_cast<uint8_t>(g * 255.0f);
        uint8_t bb = static_cast<uint8_t>(b * 255.0f);
        color = 0xFF000000 | (bb << 16) | (gg << 8) | rr;
    }
    
    task.color = color;
    
    impl_->cpuTaskStack.push(impl_->cpuTasks.size());
    impl_->cpuTasks.push_back(task);
}

void FrameProfiler::EndCpuTask() {
    if (impl_->cpuTaskStack.empty()) return;
    
    size_t taskIndex = impl_->cpuTaskStack.top();
    impl_->cpuTaskStack.pop();
    
    if (taskIndex < impl_->cpuTasks.size()) {
        impl_->cpuTasks[taskIndex].endTime = impl_->GetElapsedSeconds();
    }
}

void FrameProfiler::BeginGpuTask(const char* name, uint32_t color) {
    legit::ProfilerTask task;
    task.startTime = impl_->GetElapsedSeconds();
    task.endTime = task.startTime;
    task.name = name;
    task.color = color;
    
    impl_->gpuTaskStack.push(impl_->gpuTasks.size());
    impl_->gpuTasks.push_back(task);
}

void FrameProfiler::EndGpuTask() {
    if (impl_->gpuTaskStack.empty()) return;
    
    size_t taskIndex = impl_->gpuTaskStack.top();
    impl_->gpuTaskStack.pop();
    
    if (taskIndex < impl_->gpuTasks.size()) {
        impl_->gpuTasks[taskIndex].endTime = impl_->GetElapsedSeconds();
    }
}

// ScopedCpuTask implementation
ScopedCpuTask::ScopedCpuTask(const char* name, uint32_t color) {
    FrameProfiler::Get().BeginCpuTask(name, color);
}

ScopedCpuTask::~ScopedCpuTask() {
    FrameProfiler::Get().EndCpuTask();
}

// ScopedGpuTask implementation
ScopedGpuTask::ScopedGpuTask(const char* name, uint32_t color) {
    FrameProfiler::Get().BeginGpuTask(name, color);
}

ScopedGpuTask::~ScopedGpuTask() {
    FrameProfiler::Get().EndGpuTask();
}

} // namespace se::debug

#endif // SE_ENABLE_PROFILER
