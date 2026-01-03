#include "engine/navigation/NavigationGrid.h"
#include "engine/physics/PhysicsSystem.h"
#include "engine/Log.h"

namespace se {
namespace nav {

void NavigationGrid::Initialize(const NavigationGridSettings& settings) {
    if (initialized_) {
        SE_LOG_WARN("[NavigationGrid] Already initialized, shutting down first");
        Shutdown();
    }

    settings_ = settings;
    
    size_t nodeCount = static_cast<size_t>(settings_.width) * settings_.height;
    nodes_.resize(nodeCount);

    for (int32_t z = 0; z < settings_.height; ++z) {
        for (int32_t x = 0; x < settings_.width; ++x) {
            int32_t index = CoordToIndex(x, z);
            nodes_[index].coord = {x, z};
            nodes_[index].flags = NodeFlags::Walkable;
            nodes_[index].penalty = 0.0f;
            nodes_[index].Reset();
        }
    }

    initialized_ = true;
    SE_LOG_INFO("[NavigationGrid] Initialized {}x{} grid ({} cells, cell size: {})", 
                settings_.width, settings_.height, nodeCount, settings_.cellSize);
}

void NavigationGrid::Shutdown() {
    nodes_.clear();
    initialized_ = false;
    SE_LOG_INFO("[NavigationGrid] Shutdown");
}

void NavigationGrid::BakeObstacles(PhysicsSystem* physics) {
    if (baked_) {
        SE_LOG_DEBUG("[NavigationGrid] Already baked, skipping (call MarkDirty or RebakeObstacles to rebake)");
        return;
    }

    if (!initialized_ || !physics) {
        SE_LOG_WARN("[NavigationGrid] Cannot bake obstacles: {} initialized, physics={}",
                    initialized_ ? "is" : "not", physics ? "valid" : "null");
        return;
    }

    for (auto& node : nodes_) {
        node.flags = NodeFlags::Walkable;
    }

    int obstacleCount = 0;
    int voidCount = 0;
    int elevatedCount = 0;
    int blockedCount = 0;
    int steepCount = 0;
    
    float rayHeight = 50.0f;
    float expectedFloorY = 0.0f;
    float maxElevation = 0.3f;
    float agentHeight = 1.8f;
    float obstacleCheckStart = 0.2f;
    
    // Sample offsets within cell (center + 4 corners for diagonal detection)
    float halfCell = settings_.cellSize * 0.4f;  // Slightly inside edges
    std::vector<Vector3> sampleOffsets = {
        {0, 0, 0},                   // Center
        {-halfCell, 0, -halfCell},   // Corner 1
        {halfCell, 0, -halfCell},    // Corner 2
        {-halfCell, 0, halfCell},    // Corner 3
        {halfCell, 0, halfCell}      // Corner 4
    };

    for (int32_t z = 0; z < settings_.height; ++z) {
        for (int32_t x = 0; x < settings_.width; ++x) {
            Vector3 cellCenter = GridToWorld(x, z);
            bool isObstacle = false;
            
            // Check multiple sample points per cell
            for (const auto& offset : sampleOffsets) {
                Vector3 samplePos = cellCenter + offset;
                
                // Check for obstacles at agent level from expected floor
                Vector3 obsStart = Vector3(samplePos.x, expectedFloorY + obstacleCheckStart, samplePos.z);
                Vector3 obsEnd = Vector3(samplePos.x, expectedFloorY + agentHeight, samplePos.z);
                Vector3 obsHit, obsNormal;
                
                if (physics->Raycast(obsStart, obsEnd, obsHit, obsNormal)) {
                    isObstacle = true;
                    blockedCount++;
                    break;
                }
                
                // Find ground
                Vector3 rayStart = Vector3(samplePos.x, rayHeight, samplePos.z);
                Vector3 rayEnd   = Vector3(samplePos.x, -rayHeight, samplePos.z);
                Vector3 hitPoint, hitNormal;
                
                if (!physics->Raycast(rayStart, rayEnd, hitPoint, hitNormal)) {
                    isObstacle = true;
                    voidCount++;
                    break;
                }
                
                // Check for elevated surface
                if (hitPoint.y > expectedFloorY + maxElevation) {
                    isObstacle = true;
                    elevatedCount++;
                    break;
                }
                
                // Check steep slopes
                if (hitNormal.y < 0.5f) {
                    isObstacle = true;
                    steepCount++;
                    break;
                }
            }
            
            if (isObstacle) {
                SetObstacle(x, z, true);
                obstacleCount++;
            }
        }
    }

    int walkableCount = static_cast<int>(nodes_.size()) - obstacleCount;
    baked_ = true;
    SE_LOG_INFO("[NavigationGrid] Baked: {} walkable, {} obstacles ({} blocked, {} elevated, {} void, {} steep)", 
                walkableCount, obstacleCount, blockedCount, elevatedCount, voidCount, steepCount);
}





void NavigationGrid::RebakeObstacles(PhysicsSystem* physics) {
    baked_ = false;
    BakeObstacles(physics);
}

PathNode* NavigationGrid::GetNode(int32_t x, int32_t z) {
    if (!IsValidCoord(x, z)) return nullptr;
    return &nodes_[CoordToIndex(x, z)];
}

const PathNode* NavigationGrid::GetNode(int32_t x, int32_t z) const {
    if (!IsValidCoord(x, z)) return nullptr;
    return &nodes_[CoordToIndex(x, z)];
}

PathNode* NavigationGrid::GetNodeAt(const Vector3& worldPos) {
    GridCoord coord = WorldToGrid(worldPos);
    return GetNode(coord.x, coord.z);
}

const PathNode* NavigationGrid::GetNodeAt(const Vector3& worldPos) const {
    GridCoord coord = WorldToGrid(worldPos);
    return GetNode(coord.x, coord.z);
}

GridCoord NavigationGrid::WorldToGrid(const Vector3& worldPos) const {
    Vector3 local = worldPos - settings_.worldOrigin;
    return {
        static_cast<int32_t>(std::floor(local.x / settings_.cellSize)),
        static_cast<int32_t>(std::floor(local.z / settings_.cellSize))
    };
}

Vector3 NavigationGrid::GridToWorld(const GridCoord& coord) const {
    return GridToWorld(coord.x, coord.z);
}

Vector3 NavigationGrid::GridToWorld(int32_t x, int32_t z) const {
    float halfCell = settings_.cellSize * 0.5f;
    return settings_.worldOrigin + Vector3(
        x * settings_.cellSize + halfCell,
        0.0f,
        z * settings_.cellSize + halfCell
    );
}

bool NavigationGrid::IsValidCoord(int32_t x, int32_t z) const {
    return x >= 0 && x < settings_.width && z >= 0 && z < settings_.height;
}

bool NavigationGrid::IsValidCoord(const GridCoord& coord) const {
    return IsValidCoord(coord.x, coord.z);
}

bool NavigationGrid::IsWalkable(int32_t x, int32_t z) const {
    const PathNode* node = GetNode(x, z);
    return node && node->IsWalkable();
}

bool NavigationGrid::IsWalkable(const GridCoord& coord) const {
    return IsWalkable(coord.x, coord.z);
}

void NavigationGrid::GetNeighbors(const GridCoord& coord, std::vector<GridCoord>& outNeighbors) const {
    outNeighbors.clear();
    
    static const int32_t dx4[] = {0, 1, 0, -1};
    static const int32_t dz4[] = {1, 0, -1, 0};
    
    static const int32_t dx8[] = {0, 1, 1, 1, 0, -1, -1, -1};
    static const int32_t dz8[] = {1, 1, 0, -1, -1, -1, 0, 1};

    if (settings_.allowDiagonal) {
        for (int i = 0; i < 8; ++i) {
            GridCoord neighbor = {coord.x + dx8[i], coord.z + dz8[i]};
            if (IsValidCoord(neighbor) && IsWalkable(neighbor)) {
                outNeighbors.push_back(neighbor);
            }
        }
    } else {
        for (int i = 0; i < 4; ++i) {
            GridCoord neighbor = {coord.x + dx4[i], coord.z + dz4[i]};
            if (IsValidCoord(neighbor) && IsWalkable(neighbor)) {
                outNeighbors.push_back(neighbor);
            }
        }
    }
}

void NavigationGrid::SetObstacle(int32_t x, int32_t z, bool isObstacle) {
    PathNode* node = GetNode(x, z);
    if (!node) return;

    if (isObstacle) {
        node->flags = node->flags | NodeFlags::Obstacle;
    } else {
        node->flags = static_cast<NodeFlags>(
            static_cast<uint8_t>(node->flags) & ~static_cast<uint8_t>(NodeFlags::Obstacle)
        );
    }
}

void NavigationGrid::SetObstacle(const GridCoord& coord, bool isObstacle) {
    SetObstacle(coord.x, coord.z, isObstacle);
}

void NavigationGrid::SetObstacleRect(const GridCoord& min, const GridCoord& max, bool isObstacle) {
    for (int32_t z = min.z; z <= max.z; ++z) {
        for (int32_t x = min.x; x <= max.x; ++x) {
            SetObstacle(x, z, isObstacle);
        }
    }
}

void NavigationGrid::ClearDynamicObstacles() {
    for (auto& node : nodes_) {
        if (HasFlag(node.flags, NodeFlags::Dynamic)) {
            node.flags = static_cast<NodeFlags>(
                static_cast<uint8_t>(node.flags) & 
                ~(static_cast<uint8_t>(NodeFlags::Obstacle) | static_cast<uint8_t>(NodeFlags::Dynamic))
            );
        }
    }
}

void NavigationGrid::SetPenalty(int32_t x, int32_t z, float penalty) {
    PathNode* node = GetNode(x, z);
    if (node) {
        node->penalty = penalty;
    }
}

void NavigationGrid::SetPenalty(const GridCoord& coord, float penalty) {
    SetPenalty(coord.x, coord.z, penalty);
}

void NavigationGrid::ResetCosts() {
    for (auto& node : nodes_) {
        node.Reset();
    }
}

int32_t NavigationGrid::CoordToIndex(int32_t x, int32_t z) const {
    return z * settings_.width + x;
}

int32_t NavigationGrid::CoordToIndex(const GridCoord& coord) const {
    return CoordToIndex(coord.x, coord.z);
}

GridCoord NavigationGrid::IndexToCoord(int32_t index) const {
    return {index % settings_.width, index / settings_.width};
}

void NavigationGrid::ComputeIslands() {
    size_t nodeCount = nodes_.size();
    islandIds_.assign(nodeCount, -1);  // -1 = unvisited
    islandCount_ = 0;
    
    std::vector<GridCoord> queue;
    queue.reserve(nodeCount);
    
    // 4-directional neighbors for flood fill
    static const int32_t dx[] = {0, 1, 0, -1};
    static const int32_t dz[] = {1, 0, -1, 0};
    
    for (int32_t z = 0; z < settings_.height; ++z) {
        for (int32_t x = 0; x < settings_.width; ++x) {
            int32_t idx = CoordToIndex(x, z);
            
            // Skip if already visited or unwalkable
            if (islandIds_[idx] != -1) continue;
            if (!IsWalkable(x, z)) continue;
            
            // BFS flood fill from this cell
            int32_t currentIsland = islandCount_++;
            queue.clear();
            queue.push_back({x, z});
            islandIds_[idx] = currentIsland;
            
            size_t queueHead = 0;
            while (queueHead < queue.size()) {
                GridCoord current = queue[queueHead++];
                
                for (int i = 0; i < 4; ++i) {
                    GridCoord neighbor{current.x + dx[i], current.z + dz[i]};
                    
                    if (!IsValidCoord(neighbor)) continue;
                    if (!IsWalkable(neighbor)) continue;
                    
                    int32_t neighborIdx = CoordToIndex(neighbor);
                    if (islandIds_[neighborIdx] != -1) continue;
                    
                    islandIds_[neighborIdx] = currentIsland;
                    queue.push_back(neighbor);
                }
            }
        }
    }
    
    SE_LOG_INFO("[NavigationGrid] Computed {} islands", islandCount_);
}

int32_t NavigationGrid::GetIslandId(int32_t x, int32_t z) const {
    if (!IsValidCoord(x, z)) return -1;
    if (islandIds_.empty()) return -1;
    return islandIds_[CoordToIndex(x, z)];
}

int32_t NavigationGrid::GetIslandId(const GridCoord& coord) const {
    return GetIslandId(coord.x, coord.z);
}

bool NavigationGrid::AreConnected(const GridCoord& a, const GridCoord& b) const {
    int32_t idA = GetIslandId(a);
    int32_t idB = GetIslandId(b);
    return idA >= 0 && idA == idB;
}

}  // namespace nav
}  // namespace se
