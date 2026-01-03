#pragma once

#include "engine/navigation/PathNode.h"
#include "engine/navigation/AStar.h"

#include <glm.hpp>
#include <unordered_map>
#include <vector>
#include <functional>

// Forward declaration - Camera is in global namespace
class Camera;

namespace se {

class Entity;

namespace nav {

class NavigationGrid;
class NavigationSystem;

struct NavigationDebugSettings {
    // Master toggle
    bool enabled = false;

    // Grid visualization
    bool showGrid           = true;
    bool showWalkableCells  = true;
    bool showObstacles      = true;
    bool showDynamicObstacles = true;
    bool showCellCoords     = false;  // Text overlay (expensive)
    float gridOpacity       = 0.3f;

    // Path visualization
    bool showPaths          = true;
    bool showPathNodes      = true;
    bool showPathLines      = true;
    float pathLineWidth     = 2.0f;
    
    // Search visualization (slow, for debugging)
    bool showOpenList       = false;
    bool showClosedList     = false;
    
    // Cost visualization (displays G/H/F costs)
    bool showNodeCosts      = false;
    
    // Connectivity visualization
    bool showIslands        = false;  // Color cells by island ID

    // Colors
    glm::vec3 walkableColor     = {0.2f, 0.8f, 0.2f};  // Green
    glm::vec3 obstacleColor     = {0.8f, 0.2f, 0.2f};  // Red
    glm::vec3 dynamicObstColor  = {0.8f, 0.6f, 0.2f};  // Orange
    glm::vec3 pathColor         = {0.2f, 0.4f, 1.0f};  // Blue
    glm::vec3 waypointColor     = {1.0f, 1.0f, 0.2f};  // Yellow
    glm::vec3 openListColor     = {0.5f, 0.5f, 1.0f};  // Light blue
    glm::vec3 closedListColor   = {0.5f, 0.2f, 0.5f};  // Purple
};


// Callback for when user requests to move an agent to a location
using MoveToCallback = std::function<void(const glm::vec3& target)>;

class NavigationDebug {
   public:
    NavigationDebug() = default;
    ~NavigationDebug() = default;

    // Enable/disable with single call
    void SetEnabled(bool enabled) { settings_.enabled = enabled; }
    bool IsEnabled() const { return settings_.enabled; }
    void Toggle() { settings_.enabled = !settings_.enabled; }

    // Full settings access
    NavigationDebugSettings& GetSettings() { return settings_; }
    const NavigationDebugSettings& GetSettings() const { return settings_; }

    // Render debug visualization
    void Render(const Camera& camera, const NavigationGrid* grid);

    // Full ImGui debug panel with grid info, agent control, etc.
    void RenderImGuiPanel(NavigationSystem* navSystem, const glm::vec3& agentPosition, const Camera& camera);
    
    // Simple ImGui section (for embedding in other windows)
    void RenderImGui();

    // Set callback for when user wants to move agent to target
    void SetMoveToCallback(MoveToCallback callback) { moveToCallback_ = std::move(callback); }

    // Track active paths for visualization
    void AddActivePath(uint32_t agentId, const std::vector<glm::vec3>& path);
    void RemoveActivePath(uint32_t agentId);
    void ClearActivePaths();

    // Track search progress (for debug)
    void SetSearchState(const std::vector<int32_t>& openList,
                        const std::vector<bool>& closedSet);
    void ClearSearchState();

   private:
    void RenderGrid(const Camera& camera, const NavigationGrid* grid);
    void RenderIslands(const Camera& camera, const NavigationGrid* grid);
    void RenderPaths(const Camera& camera);
    void RenderSearchState(const Camera& camera, const NavigationGrid* grid);
    void RenderGridStats(const NavigationGrid* grid);
    void RenderAgentInfo(const glm::vec3& agentPosition, const NavigationGrid* grid);
    void RenderTargetControl();
    void RenderPathTest(NavigationSystem* navSystem, const glm::vec3& agentPosition);
    void RenderNodeCostOverlay(const Camera& camera, const NavigationGrid* grid);
    
    // Get color for island ID (deterministic palette)
    glm::vec3 GetIslandColor(int32_t islandId) const;


    NavigationDebugSettings settings_;
    MoveToCallback moveToCallback_;
    
    // Active paths being visualized
    std::unordered_map<uint32_t, std::vector<glm::vec3>> activePaths_;

    // Search state for visualization
    std::vector<int32_t> debugOpenList_;
    std::vector<bool>    debugClosedSet_;
    bool                 hasSearchState_ = false;
    
    // Target position for testing
    glm::vec3 targetPosition_{0.0f};
    bool useCustomTarget_ = false;
    
    // Last path result for display
    PathResult lastPathResult_;
    bool hasLastPathResult_ = false;
};

}  // namespace nav
}  // namespace se
