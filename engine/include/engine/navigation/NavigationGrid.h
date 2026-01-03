#pragma once

#include "engine/navigation/PathNode.h"

#include <glm.hpp>
#include <memory>
#include <vector>

namespace se {

class PhysicsSystem;

namespace nav {

using Vector3 = glm::vec3;

struct NavigationGridSettings {
    Vector3 worldOrigin{0.0f, 0.0f, 0.0f};  // World position of grid origin (bottom-left)
    int32_t width      = 100;                // Number of cells in X
    int32_t height     = 100;                // Number of cells in Z
    float   cellSize   = 1.0f;               // Size of each cell in world units
    float   agentHeight = 2.0f;              // Height for obstacle detection raycasts
    float   agentRadius = 0.5f;              // Agent radius for obstacle expansion
    bool    allowDiagonal = true;            // 8-way vs 4-way movement
};

class NavigationGrid {
   public:
    NavigationGrid() = default;
    ~NavigationGrid() = default;

    NavigationGrid(const NavigationGrid&)            = delete;
    NavigationGrid& operator=(const NavigationGrid&) = delete;
    NavigationGrid(NavigationGrid&&)                 = default;
    NavigationGrid& operator=(NavigationGrid&&)      = default;

    void Initialize(const NavigationGridSettings& settings);
    void Shutdown();

    // Bake static obstacles using physics raycasts
    void BakeObstacles(PhysicsSystem* physics);
    void RebakeObstacles(PhysicsSystem* physics);  // Force rebake
    bool IsBaked() const { return baked_; }
    void MarkDirty() { baked_ = false; }

    // Grid access
    PathNode*       GetNode(int32_t x, int32_t z);
    const PathNode* GetNode(int32_t x, int32_t z) const;
    PathNode*       GetNodeAt(const Vector3& worldPos);
    const PathNode* GetNodeAt(const Vector3& worldPos) const;

    // Coordinate conversion
    GridCoord WorldToGrid(const Vector3& worldPos) const;
    Vector3   GridToWorld(const GridCoord& coord) const;
    Vector3   GridToWorld(int32_t x, int32_t z) const;

    // Bounds checking
    bool IsValidCoord(int32_t x, int32_t z) const;
    bool IsValidCoord(const GridCoord& coord) const;
    bool IsWalkable(int32_t x, int32_t z) const;
    bool IsWalkable(const GridCoord& coord) const;

    // Neighbors
    void GetNeighbors(const GridCoord& coord, std::vector<GridCoord>& outNeighbors) const;

    // Dynamic obstacle management
    void SetObstacle(int32_t x, int32_t z, bool isObstacle);
    void SetObstacle(const GridCoord& coord, bool isObstacle);
    void SetObstacleRect(const GridCoord& min, const GridCoord& max, bool isObstacle);
    void ClearDynamicObstacles();

    // Penalty management (for variable terrain costs)
    void SetPenalty(int32_t x, int32_t z, float penalty);
    void SetPenalty(const GridCoord& coord, float penalty);

    // Getters
    const NavigationGridSettings& GetSettings() const { return settings_; }
    int32_t GetWidth() const { return settings_.width; }
    int32_t GetHeight() const { return settings_.height; }
    float   GetCellSize() const { return settings_.cellSize; }
    size_t  GetNodeCount() const { return nodes_.size(); }
    bool    IsInitialized() const { return initialized_; }

    // Reset all node costs (for new pathfinding query)
    void ResetCosts();

    // Connectivity analysis (flood fill / island detection)
    void ComputeIslands();  // Compute island IDs for all cells
    int32_t GetIslandId(int32_t x, int32_t z) const;
    int32_t GetIslandId(const GridCoord& coord) const;
    int32_t GetIslandCount() const { return islandCount_; }
    bool AreConnected(const GridCoord& a, const GridCoord& b) const;
    bool HasIslandData() const { return !islandIds_.empty(); }

    // Linear index helpers
    int32_t   CoordToIndex(int32_t x, int32_t z) const;
    int32_t   CoordToIndex(const GridCoord& coord) const;
    GridCoord IndexToCoord(int32_t index) const;

   private:
    NavigationGridSettings settings_;
    std::vector<PathNode>  nodes_;
    bool                   initialized_ = false;
    bool                   baked_       = false;
    
    // Island detection data
    std::vector<int32_t>   islandIds_;    // Island ID for each cell (-1 = obstacle/unwalkable)
    int32_t                islandCount_ = 0;
};

}  // namespace nav
}  // namespace se
