#pragma once

#include "engine/navigation/PathNode.h"

#include <glm.hpp>
#include <vector>

namespace se {
namespace nav {

using Vector3 = glm::vec3;

enum class Heuristic {
    Manhattan,   // Best for 4-way movement
    Euclidean,   // Best for 8-way movement
    Chebyshev,   // Diagonal = straight cost
    Octile       // Optimal for 8-way grids
};

struct PathfindingSettings {
    Heuristic heuristic      = Heuristic::Chebyshev;
    float     heuristicWeight = 1.0f;      // >1 = faster but less optimal (weighted A*)
    int32_t   maxIterations   = 10000;     // Prevent infinite loops
    bool      allowDiagonal   = true;
    bool      cutCorners      = false;     // Allow diagonal through corner obstacles
    float     diagonalCost    = 1.414f;    // sqrt(2)
    float     straightCost    = 1.0f;
    bool      useLOSSmoothing = true;      // Use line-of-sight smoothing (better quality)
};

struct PathResult {
    std::vector<Vector3> path;
    bool   success        = false;
    int    nodesExplored  = 0;
    float  pathLength     = 0.0f;
    float  computeTimeMs  = 0.0f;
};

class NavigationGrid;

class AStar {
   public:
    AStar() = default;
    ~AStar() = default;

    // Find path synchronously
    PathResult FindPath(
        NavigationGrid& grid,
        const Vector3& start,
        const Vector3& goal,
        const PathfindingSettings& settings = {}
    );

    // Find path from grid coordinates
    PathResult FindPath(
        NavigationGrid& grid,
        const GridCoord& start,
        const GridCoord& goal,
        const PathfindingSettings& settings = {}
    );

   private:
    float CalculateHeuristic(const GridCoord& from, const GridCoord& to, 
                             Heuristic heuristic) const;
    void  ReconstructPath(NavigationGrid& grid, int32_t endIndex, 
                          std::vector<Vector3>& outPath) const;
    void  SmoothPath(std::vector<Vector3>& path) const;
    
    // Line-of-sight based smoothing (removes unnecessary waypoints using Bresenham)
    void  SmoothPathLOS(NavigationGrid& grid, std::vector<Vector3>& path) const;
    
    // Check if straight line between two points is walkable (Bresenham line algorithm)
    bool  HasLineOfSight(const NavigationGrid& grid, const GridCoord& from, 
                         const GridCoord& to) const;
    
    // Find nearest walkable cell within radius
    GridCoord FindNearestWalkable(const NavigationGrid& grid, const GridCoord& coord, 
                                  int32_t searchRadius) const;

    // Open list as binary heap indices
    std::vector<int32_t> openList_;
    std::vector<bool>    closedSet_;
    std::vector<float>   fCosts_;  // For heap comparison
    
    // Store grid pointer for LOS smoothing (set during FindPath)
    mutable NavigationGrid* currentGrid_ = nullptr;
};

}  // namespace nav
}  // namespace se
