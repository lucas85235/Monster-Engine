#pragma once

#include <glm.hpp>
#include <unordered_map>
#include <vector>
#include <queue>
#include <cstdint>

namespace se {
namespace gi {

struct BrickKey {
    int cascadeLevel;
    glm::ivec3 coord;
    
    bool operator==(const BrickKey& other) const {
        return cascadeLevel == other.cascadeLevel &&
               coord.x == other.coord.x &&
               coord.y == other.coord.y &&
               coord.z == other.coord.z;
    }
};

struct BrickKeyHash {
    std::size_t operator()(const BrickKey& key) const {
        std::size_t h = std::hash<int>()(key.cascadeLevel);
        h ^= std::hash<int>()(key.coord.x) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>()(key.coord.y) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= std::hash<int>()(key.coord.z) + 0x9e3779b9 + (h << 6) + (h >> 2);
        return h;
    }
};

struct BrickData {
    uint32_t atlasOffset = 0;
    uint32_t lastUsedFrame = 0;
    bool dirty = true;
    bool active = false;
};

struct SparseBrickCacheConfig {
    int numCascades = 4;
    int brickSize = 8;
    int maxBricksPerCascade = 512;
    int maxTotalBricks = 2048;
    int updateBudget = 128;
    float cascadeWorldScale = 2.0f;
    float baseVoxelSize = 0.5f;
};

class SparseBrickCache {
public:
    SparseBrickCache();
    ~SparseBrickCache();
    
    void Init(const SparseBrickCacheConfig& config);
    void Shutdown();
    
    void BeginFrame(uint32_t frameNumber, const glm::vec3& cameraPos);
    void EndFrame();
    
    uint32_t AllocateBrick(const BrickKey& key);
    void FreeBrick(const BrickKey& key);
    void TouchBrick(const BrickKey& key);
    void MarkBrickDirty(const BrickKey& key);
    
    bool HasBrick(const BrickKey& key) const;
    BrickData* GetBrick(const BrickKey& key);
    const BrickData* GetBrick(const BrickKey& key) const;
    
    void GetDirtyBricks(std::vector<BrickKey>& outBricks, int maxCount);
    void GetActiveBricks(int cascadeLevel, std::vector<BrickKey>& outBricks) const;
    
    void UpdateActiveBricks(const glm::vec3& cameraPos, float activationRadius);
    void RecycleLRUBricks(int count);
    
    uint32_t GetSHAtlasTexture() const { return shAtlasTexture_; }
    uint32_t GetBetaAtlasTexture() const { return betaAtlasTexture_; }
    
    const SparseBrickCacheConfig& GetConfig() const { return config_; }
    
    int GetActiveBrickCount() const { return static_cast<int>(activeBricks_.size()); }
    int GetActiveBrickCount(int cascadeLevel) const;
    int GetDirtyBrickCount() const;
    
    glm::vec3 BrickKeyToWorldPos(const BrickKey& key) const;
    BrickKey WorldPosToBrickKey(int cascadeLevel, const glm::vec3& worldPos) const;
    float GetCascadeVoxelSize(int cascadeLevel) const;
    
private:
    void CreateAtlasTextures();
    void DestroyAtlasTextures();
    uint32_t FindFreeBrickSlot();
    
    SparseBrickCacheConfig config_;
    bool initialized_ = false;
    uint32_t currentFrame_ = 0;
    glm::vec3 cameraPos_{0.0f};
    
    std::unordered_map<BrickKey, BrickData, BrickKeyHash> brickMap_;
    std::vector<BrickKey> activeBricks_;
    std::queue<uint32_t> freeBrickSlots_;
    
    uint32_t shAtlasTexture_ = 0;
    uint32_t betaAtlasTexture_ = 0;
    
    int atlasWidth_ = 0;
    int atlasHeight_ = 0;
    int atlasDepth_ = 0;
};

}  // namespace gi
}  // namespace se
