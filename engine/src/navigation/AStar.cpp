#include "engine/navigation/AStar.h"
#include "engine/navigation/NavigationGrid.h"
#include "engine/Log.h"

#include <algorithm>
#include <chrono>
#include <cmath>

namespace se {
namespace nav {

PathResult AStar::FindPath(NavigationGrid& grid, const Vector3& start, const Vector3& goal,
                           const PathfindingSettings& settings) {
    GridCoord startCoord = grid.WorldToGrid(start);
    GridCoord goalCoord  = grid.WorldToGrid(goal);
    SE_LOG_DEBUG("[AStar] World ({:.1f},{:.1f},{:.1f}) -> Grid ({},{}) | Goal ({:.1f},{:.1f},{:.1f}) -> Grid ({},{})",
                 start.x, start.y, start.z, startCoord.x, startCoord.z,
                 goal.x, goal.y, goal.z, goalCoord.x, goalCoord.z);
    return FindPath(grid, startCoord, goalCoord, settings);
}

PathResult AStar::FindPath(NavigationGrid& grid, const GridCoord& start, const GridCoord& goal,
                           const PathfindingSettings& settings) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    PathResult result;
    result.success = false;

    if (!grid.IsValidCoord(start) || !grid.IsValidCoord(goal)) {
        SE_LOG_WARN("[AStar] Invalid start or goal coordinates");
        return result;
    }

    // Find walkable start position (fallback to nearby cells if blocked)
    GridCoord actualStart = start;
    if (!grid.IsWalkable(start)) {
        SE_LOG_WARN("[AStar] Start coord ({},{}) is not walkable, searching nearby...", start.x, start.z);
        actualStart = FindNearestWalkable(grid, start, 10);
        if (!grid.IsWalkable(actualStart)) {
            SE_LOG_WARN("[AStar] Start position and surroundings are not walkable (searched 10 cells)");
            return result;
        }
        SE_LOG_INFO("[AStar] Found walkable start at ({},{})", actualStart.x, actualStart.z);
    }

    // Find walkable goal position
    GridCoord actualGoal = goal;
    if (!grid.IsWalkable(goal)) {
        SE_LOG_WARN("[AStar] Goal coord ({},{}) is not walkable, searching nearby...", goal.x, goal.z);
        actualGoal = FindNearestWalkable(grid, goal, 10);
        if (!grid.IsWalkable(actualGoal)) {
            SE_LOG_WARN("[AStar] Goal position and surroundings are not walkable (searched 10 cells)");
            return result;
        }
        SE_LOG_INFO("[AStar] Found walkable goal at ({},{})", actualGoal.x, actualGoal.z);
    }

    if (actualStart == actualGoal) {
        result.success = true;
        result.path.push_back(grid.GridToWorld(actualGoal));
        return result;
    }

    grid.ResetCosts();


    size_t nodeCount = grid.GetNodeCount();
    openList_.clear();
    openList_.reserve(nodeCount / 4);
    closedSet_.assign(nodeCount, false);
    fCosts_.assign(nodeCount, std::numeric_limits<float>::max());

    int32_t startIndex = grid.CoordToIndex(actualStart);
    int32_t goalIndex  = grid.CoordToIndex(actualGoal);

    PathNode* startNode = grid.GetNode(actualStart.x, actualStart.z);
    startNode->gCost = 0.0f;
    startNode->hCost = CalculateHeuristic(actualStart, actualGoal, settings.heuristic) * settings.heuristicWeight;
    startNode->parentIndex = -1;

    fCosts_[startIndex] = startNode->GetFCost();
    openList_.push_back(startIndex);

    auto heapCompare = [this](int32_t a, int32_t b) {
        return fCosts_[a] > fCosts_[b];
    };

    std::vector<GridCoord> neighbors;
    neighbors.reserve(8);

    int iterations = 0;

    while (!openList_.empty() && iterations < settings.maxIterations) {
        ++iterations;

        std::pop_heap(openList_.begin(), openList_.end(), heapCompare);
        int32_t currentIndex = openList_.back();
        openList_.pop_back();

        if (currentIndex == goalIndex) {
            ReconstructPath(grid, goalIndex, result.path);
            result.success = true;
            break;
        }

        if (closedSet_[currentIndex]) {
            continue;
        }
        closedSet_[currentIndex] = true;

        GridCoord currentCoord = grid.IndexToCoord(currentIndex);
        PathNode* currentNode  = grid.GetNode(currentCoord.x, currentCoord.z);

        grid.GetNeighbors(currentCoord, neighbors);

        for (const auto& neighborCoord : neighbors) {
            int32_t neighborIndex = grid.CoordToIndex(neighborCoord);

            if (closedSet_[neighborIndex]) {
                continue;
            }

            PathNode* neighborNode = grid.GetNode(neighborCoord.x, neighborCoord.z);
            if (!neighborNode || !neighborNode->IsWalkable()) {
                continue;
            }

            bool isDiagonal = (neighborCoord.x != currentCoord.x) && (neighborCoord.z != currentCoord.z);
            float moveCost = isDiagonal ? settings.diagonalCost : settings.straightCost;
            moveCost += neighborNode->penalty;

            float tentativeG = currentNode->gCost + moveCost;

            if (tentativeG < neighborNode->gCost) {
                neighborNode->parentIndex = currentIndex;
                neighborNode->gCost = tentativeG;
                neighborNode->hCost = CalculateHeuristic(neighborCoord, actualGoal, settings.heuristic) 
                                      * settings.heuristicWeight;

                fCosts_[neighborIndex] = neighborNode->GetFCost();

                openList_.push_back(neighborIndex);
                std::push_heap(openList_.begin(), openList_.end(), heapCompare);
            }
        }
    }

    result.nodesExplored = iterations;

    if (result.success && result.path.size() > 2) {
        if (settings.useLOSSmoothing) {
            SmoothPathLOS(grid, result.path);
        } else {
            SmoothPath(result.path);
        }
    }

    if (result.success) {
        for (size_t i = 0; i < result.path.size() - 1; ++i) {
            result.pathLength += glm::distance(result.path[i], result.path[i + 1]);
        }
    }

    auto endTime = std::chrono::high_resolution_clock::now();
    result.computeTimeMs = std::chrono::duration<float, std::milli>(endTime - startTime).count();

    if (!result.success && iterations >= settings.maxIterations) {
        SE_LOG_WARN("[AStar] Path not found: max iterations ({}) reached", settings.maxIterations);
    }

    return result;
}

float AStar::CalculateHeuristic(const GridCoord& from, const GridCoord& to, Heuristic heuristic) const {
    int32_t dx = std::abs(to.x - from.x);
    int32_t dz = std::abs(to.z - from.z);

    switch (heuristic) {
        case Heuristic::Manhattan:
            return static_cast<float>(dx + dz);

        case Heuristic::Euclidean:
            return std::sqrt(static_cast<float>(dx * dx + dz * dz));

        case Heuristic::Chebyshev:
            return static_cast<float>(std::max(dx, dz));

        case Heuristic::Octile:
        default: {
            int32_t minD = std::min(dx, dz);
            int32_t maxD = std::max(dx, dz);
            return 1.414f * minD + (maxD - minD);
        }
    }
}

void AStar::ReconstructPath(NavigationGrid& grid, int32_t endIndex, std::vector<Vector3>& outPath) const {
    outPath.clear();
    
    int32_t currentIndex = endIndex;
    while (currentIndex != -1) {
        GridCoord coord = grid.IndexToCoord(currentIndex);
        outPath.push_back(grid.GridToWorld(coord));
        
        PathNode* node = grid.GetNode(coord.x, coord.z);
        currentIndex = node ? node->parentIndex : -1;
    }

    std::reverse(outPath.begin(), outPath.end());
}

void AStar::SmoothPath(std::vector<Vector3>& path) const {
    if (path.size() <= 2) return;

    std::vector<Vector3> smoothed;
    smoothed.reserve(path.size());
    smoothed.push_back(path.front());

    size_t i = 0;
    while (i < path.size() - 1) {
        size_t furthest = i + 1;
        
        for (size_t j = i + 2; j < path.size(); ++j) {
            Vector3 dir1 = glm::normalize(path[furthest] - path[i]);
            Vector3 dir2 = glm::normalize(path[j] - path[i]);
            
            if (glm::dot(dir1, dir2) > 0.98f) {
                furthest = j;
            } else {
                break;
            }
        }

        smoothed.push_back(path[furthest]);
        i = furthest;
    }

    path = std::move(smoothed);
}

GridCoord AStar::FindNearestWalkable(const NavigationGrid& grid, const GridCoord& coord, 
                                      int32_t searchRadius) const {
    // Search in expanding square rings around the coordinate
    for (int32_t r = 1; r <= searchRadius; ++r) {
        for (int32_t dx = -r; dx <= r; ++dx) {
            for (int32_t dz = -r; dz <= r; ++dz) {
                // Only check the ring, not the interior
                if (std::abs(dx) != r && std::abs(dz) != r) continue;
                
                GridCoord candidate{coord.x + dx, coord.z + dz};
                if (grid.IsValidCoord(candidate) && grid.IsWalkable(candidate)) {
                    return candidate;
                }
            }
        }
    }
    return coord;  // Return original if no walkable found
}

bool AStar::HasLineOfSight(const NavigationGrid& grid, const GridCoord& from, 
                            const GridCoord& to) const {
    // Bresenham's line algorithm to check all cells between from and to
    int32_t x0 = from.x, z0 = from.z;
    int32_t x1 = to.x, z1 = to.z;
    
    int32_t dx = std::abs(x1 - x0);
    int32_t dz = std::abs(z1 - z0);
    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t sz = (z0 < z1) ? 1 : -1;
    int32_t err = dx - dz;
    
    while (true) {
        // Check current cell
        if (!grid.IsValidCoord(x0, z0) || !grid.IsWalkable(x0, z0)) {
            return false;
        }
        
        // Reached destination
        if (x0 == x1 && z0 == z1) {
            return true;
        }
        
        int32_t e2 = 2 * err;
        if (e2 > -dz) {
            err -= dz;
            x0 += sx;
        }
        if (e2 < dx) {
            err += dx;
            z0 += sz;
        }
    }
}

void AStar::SmoothPathLOS(NavigationGrid& grid, std::vector<Vector3>& path) const {
    if (path.size() <= 2) return;
    
    std::vector<Vector3> smoothed;
    smoothed.reserve(path.size());
    smoothed.push_back(path.front());
    
    size_t current = 0;
    
    while (current < path.size() - 1) {
        // Find the furthest point we can see directly from current
        size_t furthest = current + 1;
        
        GridCoord currentCoord = grid.WorldToGrid(path[current]);
        
        for (size_t probe = current + 2; probe < path.size(); ++probe) {
            GridCoord probeCoord = grid.WorldToGrid(path[probe]);
            
            if (HasLineOfSight(grid, currentCoord, probeCoord)) {
                furthest = probe;
            } else {
                // Stop probing when we lose LOS (optimization)
                break;
            }
        }
        
        smoothed.push_back(path[furthest]);
        current = furthest;
    }
    
    SE_LOG_DEBUG("[AStar] LOS smoothing: {} -> {} waypoints", path.size(), smoothed.size());
    path = std::move(smoothed);
}

}  // namespace nav
}  // namespace se

