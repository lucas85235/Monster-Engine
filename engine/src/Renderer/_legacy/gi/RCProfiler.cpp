#include "engine/renderer/gi/RCProfiler.h"
#include "engine/Log.h"

#include <glad/glad.h>
#include <sstream>
#include <iomanip>
#include <numeric>

namespace se {
namespace gi {

RCProfiler::RCProfiler() = default;

RCProfiler::~RCProfiler() {
    Shutdown();
}

void RCProfiler::Init() {
    if (initialized_) return;
    
    frameHistory_.resize(historySize_);
    
    SE_LOG_INFO("RCProfiler initialized with {} frame history", historySize_);
    initialized_ = true;
}

void RCProfiler::Shutdown() {
    if (!initialized_) return;
    
    if (useGpuQueries_) {
        glDeleteQueries(16, gpuQueries_);
    }
    
    frameHistory_.clear();
    initialized_ = false;
}

float RCProfiler::GetElapsedMs(TimePoint start, TimePoint end) const {
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    return static_cast<float>(duration.count()) / 1000.0f;
}

void RCProfiler::BeginFrame() {
    if (!enabled_) return;
    
    currentFrameData_ = RCTimingData{};
    frameStart_ = Clock::now();
}

void RCProfiler::EndFrame() {
    if (!enabled_) return;
    
    auto frameEnd = Clock::now();
    currentFrameData_.totalMs = GetElapsedMs(frameStart_, frameEnd);
    
    frameHistory_[historyIndex_] = currentFrameData_;
    historyIndex_ = (historyIndex_ + 1) % historySize_;
    
    UpdateAverages();
}

void RCProfiler::BeginVoxelization() {
    if (!enabled_) return;
    voxelStart_ = Clock::now();
}

void RCProfiler::EndVoxelization() {
    if (!enabled_) return;
    currentFrameData_.voxelizationMs = GetElapsedMs(voxelStart_, Clock::now());
}

void RCProfiler::BeginCascadeUpdate(int cascadeLevel) {
    if (!enabled_ || cascadeLevel < 0 || cascadeLevel >= 8) return;
    cascadeStart_[cascadeLevel] = Clock::now();
}

void RCProfiler::EndCascadeUpdate(int cascadeLevel) {
    if (!enabled_ || cascadeLevel < 0 || cascadeLevel >= 8) return;
    currentFrameData_.cascadeUpdateMs[cascadeLevel] = GetElapsedMs(cascadeStart_[cascadeLevel], Clock::now());
}

void RCProfiler::BeginCascadeMerge() {
    if (!enabled_) return;
    mergeStart_ = Clock::now();
}

void RCProfiler::EndCascadeMerge() {
    if (!enabled_) return;
    currentFrameData_.cascadeMergeMs = GetElapsedMs(mergeStart_, Clock::now());
}

void RCProfiler::BeginResolve() {
    if (!enabled_) return;
    resolveStart_ = Clock::now();
}

void RCProfiler::EndResolve() {
    if (!enabled_) return;
    currentFrameData_.resolveMs = GetElapsedMs(resolveStart_, Clock::now());
}

void RCProfiler::RecordActiveBricks(int cascadeLevel, int count) {
    if (cascadeLevel >= 0 && cascadeLevel < 8) {
        currentFrameData_.activeBricks[cascadeLevel] = count;
    }
}

void RCProfiler::RecordDirtyBricks(int count) {
    currentFrameData_.dirtyBricks = count;
}

void RCProfiler::RecordUpdatedBricks(int count) {
    currentFrameData_.updatedBricks = count;
}

void RCProfiler::RecordRaysTraced(int count) {
    currentFrameData_.raysTraced = count;
}

void RCProfiler::RecordMemoryUsage(uint64_t bytes) {
    currentFrameData_.memoryUsedBytes = bytes;
}

void RCProfiler::UpdateAverages() {
    averageData_ = RCTimingData{};
    
    size_t validFrames = std::min(historyIndex_ + 1, historySize_);
    if (validFrames == 0) return;
    
    for (size_t i = 0; i < validFrames; ++i) {
        const auto& frame = frameHistory_[i];
        averageData_.totalMs += frame.totalMs;
        averageData_.voxelizationMs += frame.voxelizationMs;
        averageData_.cascadeMergeMs += frame.cascadeMergeMs;
        averageData_.resolveMs += frame.resolveMs;
        
        for (int c = 0; c < 8; ++c) {
            averageData_.cascadeUpdateMs[c] += frame.cascadeUpdateMs[c];
            averageData_.activeBricks[c] += frame.activeBricks[c];
        }
        
        averageData_.dirtyBricks += frame.dirtyBricks;
        averageData_.updatedBricks += frame.updatedBricks;
        averageData_.raysTraced += frame.raysTraced;
    }
    
    float invFrames = 1.0f / static_cast<float>(validFrames);
    averageData_.totalMs *= invFrames;
    averageData_.voxelizationMs *= invFrames;
    averageData_.cascadeMergeMs *= invFrames;
    averageData_.resolveMs *= invFrames;
    
    for (int c = 0; c < 8; ++c) {
        averageData_.cascadeUpdateMs[c] *= invFrames;
        averageData_.activeBricks[c] = static_cast<int>(averageData_.activeBricks[c] * invFrames);
    }
    
    averageData_.dirtyBricks = static_cast<int>(averageData_.dirtyBricks * invFrames);
    averageData_.updatedBricks = static_cast<int>(averageData_.updatedBricks * invFrames);
    averageData_.raysTraced = static_cast<int>(averageData_.raysTraced * invFrames);
    averageData_.memoryUsedBytes = currentFrameData_.memoryUsedBytes;
}

std::string RCProfiler::GetSummaryString() const {
    std::ostringstream ss;
    ss << std::fixed << std::setprecision(2);
    
    ss << "RC GI Profile (avg):\n";
    ss << "  Total: " << averageData_.totalMs << " ms\n";
    ss << "  Voxel: " << averageData_.voxelizationMs << " ms\n";
    
    float cascadeTotal = 0.0f;
    for (int c = 0; c < 4; ++c) {
        cascadeTotal += averageData_.cascadeUpdateMs[c];
        ss << "  Cascade " << c << ": " << averageData_.cascadeUpdateMs[c] << " ms";
        ss << " (" << averageData_.activeBricks[c] << " bricks)\n";
    }
    
    ss << "  Merge: " << averageData_.cascadeMergeMs << " ms\n";
    ss << "  Resolve: " << averageData_.resolveMs << " ms\n";
    ss << "  Dirty/Updated: " << averageData_.dirtyBricks << "/" << averageData_.updatedBricks << "\n";
    ss << "  Memory: " << (averageData_.memoryUsedBytes / (1024 * 1024)) << " MB\n";
    
    return ss.str();
}

}  // namespace gi
}  // namespace se
