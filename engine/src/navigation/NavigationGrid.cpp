#include "engine/navigation/NavigationGrid.h"
#include "engine/physics/PhysicsSystem.h"
#include "engine/Log.h"

#include <atomic>
#include <chrono>
#include <thread>
#include <algorithm>

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
        SE_LOG_DEBUG("[NavigationGrid] Already baked, skipping");
        return;
    }

    if (!initialized_ || !physics) {
        SE_LOG_WARN("[NavigationGrid] Cannot bake obstacles: {} initialized, physics={}",
                    initialized_ ? "is" : "not", physics ? "valid" : "null");
        return;
    }

    auto startTime = std::chrono::high_resolution_clock::now();

    const size_t nodeCount = nodes_.size();
    const float rayHeight = 50.0f;
    const float expectedFloorY = settings_.worldOrigin.y;
    const float agentHeight = settings_.agentHeight;
    const float agentRadius = settings_.agentRadius;
    const float obstacleThreshold = 0.5f;  // Height above floor to consider obstacle (increased from 0.15f)
    
    SE_LOG_DEBUG("[NavigationGrid] BakeObstacles: expectedFloorY={:.2f}, agentHeight={:.2f}, threshold={:.2f}",
                 expectedFloorY, agentHeight, obstacleThreshold);
    
    // Reset all nodes to walkable
    for (auto& node : nodes_) {
        node.flags = NodeFlags::Walkable;
    }

    // PHASE 1: Build ground detection rays (one per cell)
    std::vector<RaycastRequest> groundRays;
    groundRays.reserve(nodeCount);
    
    for (int32_t z = 0; z < settings_.height; ++z) {
        for (int32_t x = 0; x < settings_.width; ++x) {
            Vector3 cellCenter = GridToWorld(x, z);
            groundRays.push_back({
                Vector3(cellCenter.x, rayHeight, cellCenter.z),
                Vector3(cellCenter.x, expectedFloorY - 1.0f, cellCenter.z)
            });
        }
    }
    
    // Execute ground raycasts in batch
    std::vector<RaycastResult> groundResults;
    physics->RaycastBatch(groundRays, groundResults);
    
    // Process ground results and mark obstacles
    int obstacleCount = 0;
    int noHitCount = 0;
    int aboveFloorCount = 0;
    int steepCount = 0;
    std::vector<bool> needsObstacleCheck(nodeCount, false);
    
    for (size_t i = 0; i < nodeCount; ++i) {
        const auto& result = groundResults[i];
        int32_t x = static_cast<int32_t>(i) % settings_.width;
        int32_t z = static_cast<int32_t>(i) / settings_.width;
        
        if (!result.hit) {
            // No ground = void area
            SetObstacle(x, z, true);
            obstacleCount++;
            noHitCount++;
            continue;
        }
        
        // Check if hit surface is above floor (obstacle on top of floor)
        float heightAboveFloor = result.hitPoint.y - expectedFloorY;
        if (heightAboveFloor > obstacleThreshold) {
            SetObstacle(x, z, true);
            obstacleCount++;
            aboveFloorCount++;
            // Log first few for debugging
            if (aboveFloorCount <= 3) {
                SE_LOG_DEBUG("[NavigationGrid] Cell ({},{}) marked as obstacle: hitY={:.2f}, floorY={:.2f}, diff={:.2f}",
                             x, z, result.hitPoint.y, expectedFloorY, heightAboveFloor);
            }
            continue;
        }
        
        // Check if ground is too steep
        if (result.hitNormal.y < 0.7f) {
            SetObstacle(x, z, true);
            obstacleCount++;
            steepCount++;
            continue;
        }
        
        // Cell has valid ground, needs obstacle check above
        needsObstacleCheck[i] = true;
    }
    
    SE_LOG_DEBUG("[NavigationGrid] Phase 1 results: noHit={}, aboveFloor={}, steep={}, needsCheck={}",
                 noHitCount, aboveFloorCount, steepCount, nodeCount - obstacleCount);
    
    // PHASE 2: Build headroom check rays (only for cells with valid ground)
    // Use 5-point sampling: center + 4 corners to catch grid-aligned obstacles
    std::vector<RaycastRequest> headroomRays;
    std::vector<size_t> headroomIndices;  // Track which original cell each ray belongs to
    
    // Pre-compute sample offsets (center + 4 near-edge positions)
    const float halfCell = settings_.cellSize * 0.5f;
    const float edgeOffset = halfCell * 0.85f;  // 85% towards edge
    const Vector3 sampleOffsets[5] = {
        {0.0f, 0.0f, 0.0f},                     // Center
        {-edgeOffset, 0.0f, -edgeOffset},       // Near corner
        { edgeOffset, 0.0f, -edgeOffset},       // Near corner
        { edgeOffset, 0.0f,  edgeOffset},       // Near corner
        {-edgeOffset, 0.0f,  edgeOffset}        // Near corner
    };
    
    // Count cells that need checking
    size_t cellsToCheck = 0;
    for (size_t i = 0; i < nodeCount; ++i) {
        if (needsObstacleCheck[i]) cellsToCheck++;
    }
    headroomRays.reserve(cellsToCheck * 5);
    headroomIndices.reserve(cellsToCheck * 5);
    
    for (size_t i = 0; i < nodeCount; ++i) {
        if (!needsObstacleCheck[i]) continue;
        
        int32_t x = static_cast<int32_t>(i) % settings_.width;
        int32_t z = static_cast<int32_t>(i) / settings_.width;
        Vector3 cellCenter = GridToWorld(x, z);
        
        // Add 5 rays per cell
        for (const auto& offset : sampleOffsets) {
            Vector3 samplePos = cellCenter + offset;
            headroomRays.push_back({
                Vector3(samplePos.x, expectedFloorY + 0.05f, samplePos.z),
                Vector3(samplePos.x, expectedFloorY + agentHeight, samplePos.z)
            });
            headroomIndices.push_back(i);
        }
    }
    
    // Execute headroom raycasts in batch
    std::vector<RaycastResult> headroomResults;
    int headroomHits = 0;
    if (!headroomRays.empty()) {
        physics->RaycastBatch(headroomRays, headroomResults);
        
        // Track which cells are already marked
        std::vector<bool> cellMarked(nodeCount, false);
        
        for (size_t j = 0; j < headroomResults.size(); ++j) {
            if (headroomResults[j].hit) {
                size_t cellIdx = headroomIndices[j];
                if (!cellMarked[cellIdx]) {
                    cellMarked[cellIdx] = true;
                    int32_t x = static_cast<int32_t>(cellIdx) % settings_.width;
                    int32_t z = static_cast<int32_t>(cellIdx) / settings_.width;
                    SetObstacle(x, z, true);
                    obstacleCount++;
                    headroomHits++;
                }
            }
        }
    }
    
    SE_LOG_DEBUG("[NavigationGrid] Phase 2 results: {} cells blocked by headroom check ({} rays)", 
                 headroomHits, headroomRays.size());
    
    // PHASE 3: Agent radius expansion using grid dilation
    // Mark cells within agentRadius of obstacles as blocked
    if (agentRadius > 0.01f) {
        int32_t dilationRadius = static_cast<int32_t>(std::ceil(agentRadius / settings_.cellSize));
        
        // Create copy of current obstacle state
        std::vector<bool> originalObstacles(nodeCount, false);
        for (size_t i = 0; i < nodeCount; ++i) {
            originalObstacles[i] = !nodes_[i].IsWalkable();
        }
        
        // Dilate obstacles
        for (int32_t z = 0; z < settings_.height; ++z) {
            for (int32_t x = 0; x < settings_.width; ++x) {
                size_t idx = static_cast<size_t>(CoordToIndex(x, z));
                if (!originalObstacles[idx]) continue;  // Skip non-obstacles
                
                // Mark neighbors within radius
                for (int32_t dz = -dilationRadius; dz <= dilationRadius; ++dz) {
                    for (int32_t dx = -dilationRadius; dx <= dilationRadius; ++dx) {
                        if (dx == 0 && dz == 0) continue;
                        
                        int32_t nx = x + dx;
                        int32_t nz = z + dz;
                        if (!IsValidCoord(nx, nz)) continue;
                        
                        // Check if within circular radius
                        float dist = std::sqrt(static_cast<float>(dx * dx + dz * dz)) * settings_.cellSize;
                        if (dist <= agentRadius) {
                            size_t nIdx = static_cast<size_t>(CoordToIndex(nx, nz));
                            if (!originalObstacles[nIdx] && nodes_[nIdx].IsWalkable()) {
                                SetObstacle(nx, nz, true);
                                obstacleCount++;
                            }
                        }
                    }
                }
            }
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    float elapsedMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();

    int walkableCount = static_cast<int>(nodes_.size()) - obstacleCount;
    baked_ = true;
    SE_LOG_INFO("[NavigationGrid] Baked in {:.1f}ms: {} walkable, {} obstacles (batch raycast)", 
                elapsedMs, walkableCount, obstacleCount);
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
