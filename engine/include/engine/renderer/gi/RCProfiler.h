#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <cstdint>

namespace se {
namespace gi {

struct RCTimingData {
    float voxelizationMs = 0.0f;
    float cascadeUpdateMs[8] = {0};
    float cascadeMergeMs = 0.0f;
    float resolveMs = 0.0f;
    float totalMs = 0.0f;
    
    int activeBricks[8] = {0};
    int dirtyBricks = 0;
    int updatedBricks = 0;
    int raysTraced = 0;
    
    uint64_t memoryUsedBytes = 0;
};

class RCProfiler {
public:
    RCProfiler();
    ~RCProfiler();
    
    void Init();
    void Shutdown();
    
    void BeginFrame();
    void EndFrame();
    
    void BeginVoxelization();
    void EndVoxelization();
    
    void BeginCascadeUpdate(int cascadeLevel);
    void EndCascadeUpdate(int cascadeLevel);
    
    void BeginCascadeMerge();
    void EndCascadeMerge();
    
    void BeginResolve();
    void EndResolve();
    
    void RecordActiveBricks(int cascadeLevel, int count);
    void RecordDirtyBricks(int count);
    void RecordUpdatedBricks(int count);
    void RecordRaysTraced(int count);
    void RecordMemoryUsage(uint64_t bytes);
    
    const RCTimingData& GetCurrentFrameData() const { return currentFrameData_; }
    const RCTimingData& GetAverageData() const { return averageData_; }
    
    float GetAverageTotalMs() const { return averageData_.totalMs; }
    float GetAverageVoxelizationMs() const { return averageData_.voxelizationMs; }
    
    void SetEnabled(bool enabled) { enabled_ = enabled; }
    bool IsEnabled() const { return enabled_; }
    
    std::string GetSummaryString() const;
    
private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = Clock::time_point;
    
    void UpdateAverages();
    float GetElapsedMs(TimePoint start, TimePoint end) const;
    
    bool initialized_ = false;
    bool enabled_ = true;
    
    RCTimingData currentFrameData_;
    RCTimingData averageData_;
    std::vector<RCTimingData> frameHistory_;
    size_t historySize_ = 60;
    size_t historyIndex_ = 0;
    
    TimePoint frameStart_;
    TimePoint voxelStart_;
    TimePoint cascadeStart_[8];
    TimePoint mergeStart_;
    TimePoint resolveStart_;
    
    uint32_t gpuQueries_[16] = {0};
    bool useGpuQueries_ = false;
};

}  // namespace gi
}  // namespace se
