#include "engine/renderer/gi/SparseBrickCache.h"
#include "engine/Log.h"

#include <glad/glad.h>
#include <algorithm>
#include <cmath>

namespace se {
namespace gi {

SparseBrickCache::SparseBrickCache() = default;

SparseBrickCache::~SparseBrickCache() {
    Shutdown();
}

void SparseBrickCache::Init(const SparseBrickCacheConfig& config) {
    if (initialized_) {
        SE_LOG_WARN("SparseBrickCache already initialized");
        return;
    }
    
    config_ = config;
    
    SE_LOG_INFO("Initializing SparseBrickCache: {} cascades, {} bricks/cascade, {}³ probes/brick",
                config_.numCascades, config_.maxBricksPerCascade, config_.brickSize);
    
    CreateAtlasTextures();
    
    for (uint32_t i = 0; i < static_cast<uint32_t>(config_.maxTotalBricks); ++i) {
        freeBrickSlots_.push(i);
    }
    
    initialized_ = true;
}

void SparseBrickCache::Shutdown() {
    if (!initialized_) return;
    
    SE_LOG_INFO("Shutting down SparseBrickCache");
    
    DestroyAtlasTextures();
    brickMap_.clear();
    activeBricks_.clear();
    while (!freeBrickSlots_.empty()) freeBrickSlots_.pop();
    
    initialized_ = false;
}

void SparseBrickCache::CreateAtlasTextures() {
    const int probesPerBrick = config_.brickSize * config_.brickSize * config_.brickSize;
    const int shCoeffsPerProbe = 9 * 3;
    
    int bricksPerRow = static_cast<int>(std::ceil(std::sqrt(static_cast<float>(config_.maxTotalBricks))));
    atlasWidth_ = bricksPerRow * config_.brickSize;
    atlasHeight_ = bricksPerRow * config_.brickSize;
    atlasDepth_ = config_.brickSize;
    
    glGenTextures(1, &shAtlasTexture_);
    glBindTexture(GL_TEXTURE_3D, shAtlasTexture_);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_RGBA32F, 
                 atlasWidth_ * 3, atlasHeight_ * 3, atlasDepth_,
                 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    glGenTextures(1, &betaAtlasTexture_);
    glBindTexture(GL_TEXTURE_3D, betaAtlasTexture_);
    glTexImage3D(GL_TEXTURE_3D, 0, GL_R16F,
                 atlasWidth_, atlasHeight_, atlasDepth_,
                 0, GL_RED, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    
    glBindTexture(GL_TEXTURE_3D, 0);
    
    SE_LOG_INFO("Created brick atlas textures: SH={}x{}x{}, Beta={}x{}x{}",
                atlasWidth_ * 3, atlasHeight_ * 3, atlasDepth_,
                atlasWidth_, atlasHeight_, atlasDepth_);
}

void SparseBrickCache::DestroyAtlasTextures() {
    if (shAtlasTexture_ != 0) {
        glDeleteTextures(1, &shAtlasTexture_);
        shAtlasTexture_ = 0;
    }
    if (betaAtlasTexture_ != 0) {
        glDeleteTextures(1, &betaAtlasTexture_);
        betaAtlasTexture_ = 0;
    }
}

void SparseBrickCache::BeginFrame(uint32_t frameNumber, const glm::vec3& cameraPos) {
    currentFrame_ = frameNumber;
    cameraPos_ = cameraPos;
}

void SparseBrickCache::EndFrame() {
}

uint32_t SparseBrickCache::FindFreeBrickSlot() {
    if (freeBrickSlots_.empty()) {
        RecycleLRUBricks(config_.updateBudget / 4);
    }
    
    if (freeBrickSlots_.empty()) {
        SE_LOG_WARN("No free brick slots available");
        return UINT32_MAX;
    }
    
    uint32_t slot = freeBrickSlots_.front();
    freeBrickSlots_.pop();
    return slot;
}

uint32_t SparseBrickCache::AllocateBrick(const BrickKey& key) {
    auto it = brickMap_.find(key);
    if (it != brickMap_.end()) {
        it->second.lastUsedFrame = currentFrame_;
        it->second.active = true;
        return it->second.atlasOffset;
    }
    
    uint32_t slot = FindFreeBrickSlot();
    if (slot == UINT32_MAX) {
        return UINT32_MAX;
    }
    
    BrickData data;
    data.atlasOffset = slot;
    data.lastUsedFrame = currentFrame_;
    data.dirty = true;
    data.active = true;
    
    brickMap_[key] = data;
    activeBricks_.push_back(key);
    
    return slot;
}

void SparseBrickCache::FreeBrick(const BrickKey& key) {
    auto it = brickMap_.find(key);
    if (it == brickMap_.end()) return;
    
    freeBrickSlots_.push(it->second.atlasOffset);
    brickMap_.erase(it);
    
    activeBricks_.erase(
        std::remove(activeBricks_.begin(), activeBricks_.end(), key),
        activeBricks_.end()
    );
}

void SparseBrickCache::TouchBrick(const BrickKey& key) {
    auto it = brickMap_.find(key);
    if (it != brickMap_.end()) {
        it->second.lastUsedFrame = currentFrame_;
    }
}

void SparseBrickCache::MarkBrickDirty(const BrickKey& key) {
    auto it = brickMap_.find(key);
    if (it != brickMap_.end()) {
        it->second.dirty = true;
    }
}

bool SparseBrickCache::HasBrick(const BrickKey& key) const {
    return brickMap_.find(key) != brickMap_.end();
}

BrickData* SparseBrickCache::GetBrick(const BrickKey& key) {
    auto it = brickMap_.find(key);
    return (it != brickMap_.end()) ? &it->second : nullptr;
}

const BrickData* SparseBrickCache::GetBrick(const BrickKey& key) const {
    auto it = brickMap_.find(key);
    return (it != brickMap_.end()) ? &it->second : nullptr;
}

void SparseBrickCache::GetDirtyBricks(std::vector<BrickKey>& outBricks, int maxCount) {
    outBricks.clear();
    outBricks.reserve(maxCount);
    
    for (auto& [key, data] : brickMap_) {
        if (data.dirty && data.active) {
            outBricks.push_back(key);
            if (static_cast<int>(outBricks.size()) >= maxCount) break;
        }
    }
}

void SparseBrickCache::GetActiveBricks(int cascadeLevel, std::vector<BrickKey>& outBricks) const {
    outBricks.clear();
    
    for (const auto& key : activeBricks_) {
        if (key.cascadeLevel == cascadeLevel) {
            outBricks.push_back(key);
        }
    }
}

int SparseBrickCache::GetActiveBrickCount(int cascadeLevel) const {
    int count = 0;
    for (const auto& key : activeBricks_) {
        if (key.cascadeLevel == cascadeLevel) {
            ++count;
        }
    }
    return count;
}

int SparseBrickCache::GetDirtyBrickCount() const {
    int count = 0;
    for (const auto& [key, data] : brickMap_) {
        if (data.dirty) ++count;
    }
    return count;
}

void SparseBrickCache::UpdateActiveBricks(const glm::vec3& cameraPos, float activationRadius) {
    for (auto& [key, data] : brickMap_) {
        glm::vec3 brickWorldPos = BrickKeyToWorldPos(key);
        float distance = glm::length(brickWorldPos - cameraPos);
        
        float cascadeRadius = activationRadius * std::pow(config_.cascadeWorldScale, key.cascadeLevel);
        data.active = (distance <= cascadeRadius);
    }
    
    activeBricks_.clear();
    for (const auto& [key, data] : brickMap_) {
        if (data.active) {
            activeBricks_.push_back(key);
        }
    }
}

void SparseBrickCache::RecycleLRUBricks(int count) {
    if (brickMap_.empty()) return;
    
    std::vector<std::pair<BrickKey, uint32_t>> sortedBricks;
    sortedBricks.reserve(brickMap_.size());
    
    for (const auto& [key, data] : brickMap_) {
        if (!data.active) {
            sortedBricks.emplace_back(key, data.lastUsedFrame);
        }
    }
    
    std::sort(sortedBricks.begin(), sortedBricks.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });
    
    int recycled = 0;
    for (const auto& [key, frame] : sortedBricks) {
        if (recycled >= count) break;
        FreeBrick(key);
        ++recycled;
    }
    
    if (recycled > 0) {
        SE_LOG_DEBUG("Recycled {} LRU bricks", recycled);
    }
}

float SparseBrickCache::GetCascadeVoxelSize(int cascadeLevel) const {
    return config_.baseVoxelSize * std::pow(config_.cascadeWorldScale, cascadeLevel);
}

glm::vec3 SparseBrickCache::BrickKeyToWorldPos(const BrickKey& key) const {
    float voxelSize = GetCascadeVoxelSize(key.cascadeLevel);
    float brickWorldSize = voxelSize * config_.brickSize;
    
    return glm::vec3(key.coord) * brickWorldSize + glm::vec3(brickWorldSize * 0.5f);
}

BrickKey SparseBrickCache::WorldPosToBrickKey(int cascadeLevel, const glm::vec3& worldPos) const {
    float voxelSize = GetCascadeVoxelSize(cascadeLevel);
    float brickWorldSize = voxelSize * config_.brickSize;
    
    BrickKey key;
    key.cascadeLevel = cascadeLevel;
    key.coord = glm::ivec3(glm::floor(worldPos / brickWorldSize));
    return key;
}

}  // namespace gi
}  // namespace se
