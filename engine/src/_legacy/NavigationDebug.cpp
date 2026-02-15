#include "engine/navigation/NavigationDebug.h"
#include "engine/navigation/NavigationGrid.h"
#include "engine/navigation/NavigationSystem.h"
#include "engine/debug/DebugRenderer.h"
#include "engine/Camera.h"
#include "engine/Log.h"

#include <imgui.h>
#include <cmath>

namespace se {
namespace nav {

void NavigationDebug::Render(const Camera& camera, const NavigationGrid* grid) {
    if (!settings_.enabled) return;
    if (!grid || !grid->IsInitialized()) return;

    if (settings_.showGrid) {
        RenderGrid(camera, grid);
    }
    
    if (settings_.showIslands && grid->HasIslandData()) {
        RenderIslands(camera, grid);
    }

    if (settings_.showPaths) {
        RenderPaths(camera);
    }

    if (settings_.showOpenList || settings_.showClosedList) {
        RenderSearchState(camera, grid);
    }
    
    if (settings_.showNodeCosts) {
        RenderNodeCostOverlay(camera, grid);
    }
}


void NavigationDebug::RenderImGuiPanel(NavigationSystem* navSystem, const glm::vec3& agentPosition, const Camera& camera) {
    ImGui::Begin("Navigation Debug", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    
    ImGui::Checkbox("Enable Visualization", &settings_.enabled);
    ImGui::Separator();
    
    NavigationGrid* grid = navSystem ? navSystem->GetGrid() : nullptr;
    
    // Draw 3D debug visualization using real camera
    if (settings_.enabled && grid) {
        Render(camera, grid);
    }
    
    if (ImGui::CollapsingHeader("Grid Info", ImGuiTreeNodeFlags_DefaultOpen)) {
        RenderGridStats(grid);
    }
    
    if (ImGui::CollapsingHeader("Agent Info", ImGuiTreeNodeFlags_DefaultOpen)) {
        RenderAgentInfo(agentPosition, grid);
    }
    
    if (ImGui::CollapsingHeader("Target Control", ImGuiTreeNodeFlags_DefaultOpen)) {
        RenderTargetControl();
    }
    
    if (ImGui::CollapsingHeader("Path Test")) {
        RenderPathTest(navSystem, agentPosition);
    }
    
    if (ImGui::CollapsingHeader("Node Inspector")) {
        if (grid && grid->IsInitialized()) {
            static int inspectX = 0, inspectZ = 0;
            ImGui::DragInt("Cell X", &inspectX, 0.5f, 0, grid->GetWidth() - 1);
            ImGui::DragInt("Cell Z", &inspectZ, 0.5f, 0, grid->GetHeight() - 1);
            
            const PathNode* node = grid->GetNode(inspectX, inspectZ);
            if (node) {
                ImGui::Separator();
                glm::vec3 worldPos = grid->GridToWorld(inspectX, inspectZ);
                ImGui::Text("World Pos: (%.2f, %.2f, %.2f)", worldPos.x, worldPos.y, worldPos.z);
                
                bool walkable = node->IsWalkable();
                ImVec4 statusColor = walkable ? ImVec4(0.2f, 0.8f, 0.2f, 1.0f) : ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
                ImGui::TextColored(statusColor, "Walkable: %s", walkable ? "YES" : "NO");
                
                ImGui::Text("Flags:");
                if (HasFlag(node->flags, NodeFlags::Obstacle)) ImGui::BulletText("Obstacle");
                if (HasFlag(node->flags, NodeFlags::Dynamic)) ImGui::BulletText("Dynamic");
                if (HasFlag(node->flags, NodeFlags::Water)) ImGui::BulletText("Water");
                if (HasFlag(node->flags, NodeFlags::Road)) ImGui::BulletText("Road");
                
                ImGui::Separator();
                ImGui::Text("Pathfinding Costs:");
                ImGui::Text("  G Cost (from start): %.2f", node->gCost);
                ImGui::Text("  H Cost (heuristic):  %.2f", node->hCost);
                ImGui::Text("  F Cost (total):      %.2f", node->GetFCost());
                ImGui::Text("  Penalty:             %.2f", node->penalty);
                ImGui::Text("  Parent Index:        %d", node->parentIndex);
                
                if (grid->HasIslandData()) {
                    int32_t islandId = grid->GetIslandId(inspectX, inspectZ);
                    ImGui::Text("  Island ID:           %d", islandId);
                }
                
                DebugRenderer::Get().DrawSphere(worldPos + glm::vec3(0, 0.3f, 0), 0.25f, DebugColors::Magenta, 8);
            } else {
                ImGui::TextColored(ImVec4(1, 0, 0, 1), "Invalid cell coordinates");
            }
        } else {
            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Grid not available");
        }
    }
    
    if (ImGui::CollapsingHeader("Visualization")) {
        ImGui::Text("Grid Visualization");
        ImGui::Checkbox("Show Grid", &settings_.showGrid);
        if (settings_.showGrid) {
            ImGui::Indent();
            ImGui::Checkbox("Walkable Cells", &settings_.showWalkableCells);
            ImGui::Checkbox("Obstacles", &settings_.showObstacles);
            ImGui::Checkbox("Dynamic Obstacles", &settings_.showDynamicObstacles);
            ImGui::Checkbox("Show Node Costs", &settings_.showNodeCosts);
            ImGui::SliderFloat("Grid Opacity", &settings_.gridOpacity, 0.0f, 1.0f);
            ImGui::Unindent();
        }

        ImGui::Separator();
        ImGui::Text("Path Visualization");
        ImGui::Checkbox("Show Paths", &settings_.showPaths);
        if (settings_.showPaths) {
            ImGui::Indent();
            ImGui::Checkbox("Path Nodes", &settings_.showPathNodes);
            ImGui::Checkbox("Path Lines", &settings_.showPathLines);
            ImGui::Unindent();
        }
        
        ImGui::Separator();
        ImGui::Text("Search Debug");
        ImGui::Checkbox("Show Open List", &settings_.showOpenList);
        ImGui::Checkbox("Show Closed List", &settings_.showClosedList);
        ImGui::Checkbox("Show Islands", &settings_.showIslands);

        ImGui::Separator();
        ImGui::Text("Colors");
        ImGui::ColorEdit3("Walkable", &settings_.walkableColor.x);
        ImGui::ColorEdit3("Obstacle", &settings_.obstacleColor.x);
        ImGui::ColorEdit3("Path", &settings_.pathColor.x);
    }
    
    if (ImGui::CollapsingHeader("Active Paths")) {
        ImGui::Text("Tracked Paths: %zu", activePaths_.size());
        for (const auto& [agentId, path] : activePaths_) {
            ImGui::Text("  Agent %u: %zu waypoints", agentId, path.size());
            if (!path.empty()) {
                ImGui::Text("    Current: (%.1f, %.1f, %.1f)", 
                           path.front().x, path.front().y, path.front().z);
                ImGui::Text("    Target: (%.1f, %.1f, %.1f)", 
                           path.back().x, path.back().y, path.back().z);
            }
        }
        
        if (ImGui::Button("Clear All Paths")) {
            ClearActivePaths();
        }
    }
    
    if (ImGui::CollapsingHeader("Statistics")) {
        if (navSystem) {
            const auto& stats = navSystem->GetStats();
            ImGui::Text("Total Requests:     %zu", stats.totalRequests);
            ImGui::Text("Completed:          %zu", stats.completedRequests);
            ImGui::Text("Failed:             %zu", stats.failedRequests);
            ImGui::Text("Pending:            %zu", stats.pendingRequests);
            ImGui::Text("Cache Hits:         %zu", stats.cacheHits);
            ImGui::Text("Avg Compute Time:   %.3f ms", stats.avgComputeTimeMs);
            
            if (ImGui::Button("Reset Stats")) {
                navSystem->ResetStats();
            }
        }
    }
    
    ImGui::End();
}


void NavigationDebug::RenderGridStats(const NavigationGrid* grid) {
    if (!grid || !grid->IsInitialized()) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Grid not initialized!");
        return;
    }
    
    const auto& settings = grid->GetSettings();
    
    ImGui::Text("Grid Size: %d x %d", settings.width, settings.height);
    ImGui::Text("Cell Size: %.2f", settings.cellSize);
    ImGui::Text("World Origin: (%.1f, %.1f, %.1f)", 
               settings.worldOrigin.x, settings.worldOrigin.y, settings.worldOrigin.z);
    
    glm::vec3 worldMax = settings.worldOrigin + 
                         glm::vec3(settings.width * settings.cellSize, 0, 
                                  settings.height * settings.cellSize);
    ImGui::Text("World Bounds: (%.1f, %.1f) to (%.1f, %.1f)", 
               settings.worldOrigin.x, settings.worldOrigin.z,
               worldMax.x, worldMax.z);
    
    int walkable = 0, obstacles = 0, dynamic = 0;
    for (int z = 0; z < grid->GetHeight(); ++z) {
        for (int x = 0; x < grid->GetWidth(); ++x) {
            const PathNode* node = grid->GetNode(x, z);
            if (!node) continue;
            
            if (HasFlag(node->flags, NodeFlags::Dynamic)) {
                dynamic++;
            } else if (HasFlag(node->flags, NodeFlags::Obstacle)) {
                obstacles++;
            } else if (node->IsWalkable()) {
                walkable++;
            }
        }
    }
    
    int total = settings.width * settings.height;
    ImGui::Text("Cells: %d total", total);
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 0.2f, 1), "  Walkable: %d (%.1f%%)", 
                      walkable, 100.0f * walkable / total);
    ImGui::TextColored(ImVec4(0.8f, 0.2f, 0.2f, 1), "  Obstacles: %d (%.1f%%)", 
                      obstacles, 100.0f * obstacles / total);
    ImGui::TextColored(ImVec4(0.8f, 0.6f, 0.2f, 1), "  Dynamic: %d", dynamic);
}

void NavigationDebug::RenderAgentInfo(const glm::vec3& agentPosition, const NavigationGrid* grid) {
    ImGui::Text("World Position: (%.2f, %.2f, %.2f)", 
               agentPosition.x, agentPosition.y, agentPosition.z);
    
    if (grid && grid->IsInitialized()) {
        GridCoord coord = grid->WorldToGrid(agentPosition);
        ImGui::Text("Grid Coord: (%d, %d)", coord.x, coord.z);
        
        bool validCoord = grid->IsValidCoord(coord);
        bool walkable = validCoord && grid->IsWalkable(coord);
        
        if (!validCoord) {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Status: OUT OF BOUNDS");
        } else if (!walkable) {
            ImGui::TextColored(ImVec4(1, 0.5f, 0, 1), "Status: ON OBSTACLE");
        } else {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Status: WALKABLE");
        }
        
        // Draw agent marker in world
        DebugRenderer::Get().DrawSphere(agentPosition, 0.5f, DebugColors::Cyan, 8);
        
        ImGui::Text("Nearby cells (5x5):");
        for (int dz = -2; dz <= 2; ++dz) {
            char row[64] = "";
            for (int dx = -2; dx <= 2; ++dx) {
                GridCoord check{coord.x + dx, coord.z + dz};
                char symbol = '?';
                if (!grid->IsValidCoord(check)) {
                    symbol = 'X';
                } else if (dx == 0 && dz == 0) {
                    symbol = '@';
                } else if (grid->IsWalkable(check)) {
                    symbol = '.';
                } else {
                    symbol = '#';
                }
                strncat(row, &symbol, 1);
                strncat(row, " ", 1);
            }
            ImGui::Text("  %s", row);
        }
        ImGui::Text("  Legend: @=Agent, .=Walkable, #=Blocked, X=Out");
    }
}

void NavigationDebug::RenderTargetControl() {
    ImGui::Checkbox("Use Custom Target", &useCustomTarget_);
    
    if (useCustomTarget_) {
        ImGui::DragFloat3("Target Position", &targetPosition_.x, 0.5f);
        
        if (ImGui::Button("Send Agent To Target")) {
            if (moveToCallback_) {
                moveToCallback_(targetPosition_);
            }
        }
        
        ImGui::SameLine();
        if (ImGui::Button("Set to Origin")) {
            targetPosition_ = glm::vec3(0.0f);
        }
        
        // Draw target marker
        DebugRenderer::Get().DrawSphere(targetPosition_, 0.3f, DebugColors::Yellow, 8);
        DebugRenderer::Get().DrawPoint(targetPosition_, 0.5f, DebugColors::Yellow);
    }
    
    ImGui::Text("Quick Targets:");
    if (ImGui::Button("(0, 0, 0)")) {
        targetPosition_ = glm::vec3(0, 0, 0);
        useCustomTarget_ = true;
        if (moveToCallback_) moveToCallback_(targetPosition_);
    }
    ImGui::SameLine();
    if (ImGui::Button("(5, 0, 5)")) {
        targetPosition_ = glm::vec3(5, 0, 5);
        useCustomTarget_ = true;
        if (moveToCallback_) moveToCallback_(targetPosition_);
    }
    ImGui::SameLine();
    if (ImGui::Button("(10, 0, 10)")) {
        targetPosition_ = glm::vec3(10, 0, 10);
        useCustomTarget_ = true;
        if (moveToCallback_) moveToCallback_(targetPosition_);
    }
    
    if (ImGui::Button("(-5, 0, -5)")) {
        targetPosition_ = glm::vec3(-5, 0, -5);
        useCustomTarget_ = true;
        if (moveToCallback_) moveToCallback_(targetPosition_);
    }
    ImGui::SameLine();
    if (ImGui::Button("(15, 0, 15)")) {
        targetPosition_ = glm::vec3(15, 0, 15);
        useCustomTarget_ = true;
        if (moveToCallback_) moveToCallback_(targetPosition_);
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
        if (moveToCallback_) moveToCallback_(glm::vec3(std::numeric_limits<float>::quiet_NaN()));
    }
}

void NavigationDebug::RenderPathTest(NavigationSystem* navSystem, const glm::vec3& agentPosition) {
    if (!navSystem || !navSystem->GetGrid()) {
        ImGui::TextColored(ImVec4(1, 0, 0, 1), "Navigation system not available");
        return;
    }
    
    static glm::vec3 testGoal{10.0f, 0.0f, 10.0f};
    ImGui::DragFloat3("Test Goal", &testGoal.x, 0.5f);
    
    if (ImGui::Button("Test Pathfinding")) {
        auto result = navSystem->FindPathSync(agentPosition, testGoal);
        lastPathResult_ = result;
        hasLastPathResult_ = true;
        
        if (result.success) {
            AddActivePath(999, result.path);
        }
    }
    
    ImGui::SameLine();
    if (ImGui::Button("Clear Test Path")) {
        RemoveActivePath(999);
        hasLastPathResult_ = false;
    }
    
    if (hasLastPathResult_) {
        ImGui::Separator();
        if (lastPathResult_.success) {
            ImGui::TextColored(ImVec4(0, 1, 0, 1), "Path Found!");
            ImGui::Text("  Waypoints: %zu", lastPathResult_.path.size());
            ImGui::Text("  Length: %.2f units", lastPathResult_.pathLength);
            ImGui::Text("  Nodes explored: %d", lastPathResult_.nodesExplored);
            ImGui::Text("  Compute time: %.3f ms", lastPathResult_.computeTimeMs);
        } else {
            ImGui::TextColored(ImVec4(1, 0, 0, 1), "Path NOT Found!");
            ImGui::Text("  Nodes explored: %d", lastPathResult_.nodesExplored);
            ImGui::Text("  Compute time: %.3f ms", lastPathResult_.computeTimeMs);
        }
    }
}

void NavigationDebug::RenderImGui() {
    if (!ImGui::CollapsingHeader("Navigation Debug")) return;

    ImGui::Checkbox("Enable Debug Visualization", &settings_.enabled);
    
    if (!settings_.enabled) return;

    ImGui::Separator();
    ImGui::Text("Grid Visualization");
    ImGui::Checkbox("Show Grid", &settings_.showGrid);
    if (settings_.showGrid) {
        ImGui::Indent();
        ImGui::Checkbox("Walkable Cells", &settings_.showWalkableCells);
        ImGui::Checkbox("Obstacles", &settings_.showObstacles);
        ImGui::Checkbox("Dynamic Obstacles", &settings_.showDynamicObstacles);
        ImGui::Checkbox("Cell Coordinates", &settings_.showCellCoords);
        ImGui::SliderFloat("Grid Opacity", &settings_.gridOpacity, 0.0f, 1.0f);
        ImGui::Unindent();
    }

    ImGui::Separator();
    ImGui::Text("Path Visualization");
    ImGui::Checkbox("Show Paths", &settings_.showPaths);
    if (settings_.showPaths) {
        ImGui::Indent();
        ImGui::Checkbox("Path Nodes", &settings_.showPathNodes);
        ImGui::Checkbox("Path Lines", &settings_.showPathLines);
        ImGui::SliderFloat("Line Width", &settings_.pathLineWidth, 1.0f, 5.0f);
        ImGui::Unindent();
    }

    ImGui::Separator();
    ImGui::Text("Search Debug (Slow!)");
    ImGui::Checkbox("Show Open List", &settings_.showOpenList);
    ImGui::Checkbox("Show Closed List", &settings_.showClosedList);

    ImGui::Separator();
    ImGui::Text("Colors");
    ImGui::ColorEdit3("Walkable", &settings_.walkableColor.x);
    ImGui::ColorEdit3("Obstacle", &settings_.obstacleColor.x);
    ImGui::ColorEdit3("Dynamic", &settings_.dynamicObstColor.x);
    ImGui::ColorEdit3("Path", &settings_.pathColor.x);
    ImGui::ColorEdit3("Waypoint", &settings_.waypointColor.x);

    ImGui::Separator();
    ImGui::Text("Active Paths: %zu", activePaths_.size());
}

void NavigationDebug::AddActivePath(uint32_t agentId, const std::vector<glm::vec3>& path) {
    activePaths_[agentId] = path;
}

void NavigationDebug::RemoveActivePath(uint32_t agentId) {
    activePaths_.erase(agentId);
}

void NavigationDebug::ClearActivePaths() {
    activePaths_.clear();
}

void NavigationDebug::SetSearchState(const std::vector<int32_t>& openList,
                                     const std::vector<bool>& closedSet) {
    debugOpenList_ = openList;
    debugClosedSet_ = closedSet;
    hasSearchState_ = true;
}

void NavigationDebug::ClearSearchState() {
    debugOpenList_.clear();
    debugClosedSet_.clear();
    hasSearchState_ = false;
}

void NavigationDebug::RenderGrid(const Camera& camera, const NavigationGrid* grid) {
    if (!grid) return;

    const auto& gridSettings = grid->GetSettings();
    float cellSize = gridSettings.cellSize;
    float halfCell = cellSize * 0.5f;
    float yOffset  = 0.05f;

    // Only render a reasonable area around center (full grid is too expensive)
    int maxCellsToRender = 50;  // Render up to 50x50 area
    int startX = std::max(0, gridSettings.width / 2 - maxCellsToRender / 2);
    int startZ = std::max(0, gridSettings.height / 2 - maxCellsToRender / 2);
    int endX = std::min(gridSettings.width, startX + maxCellsToRender);
    int endZ = std::min(gridSettings.height, startZ + maxCellsToRender);

    for (int32_t z = startZ; z < endZ; ++z) {
        for (int32_t x = startX; x < endX; ++x) {
            const PathNode* node = grid->GetNode(x, z);
            if (!node) continue;

            glm::vec3 worldPos = grid->GridToWorld(x, z);
            worldPos.y += yOffset;

            glm::vec3 color;
            bool shouldDraw = false;

            if (HasFlag(node->flags, NodeFlags::Dynamic)) {
                if (settings_.showDynamicObstacles) {
                    color = settings_.dynamicObstColor;
                    shouldDraw = true;
                }
            } else if (HasFlag(node->flags, NodeFlags::Obstacle)) {
                if (settings_.showObstacles) {
                    color = settings_.obstacleColor;
                    shouldDraw = true;
                }
            } else if (node->IsWalkable()) {
                if (settings_.showWalkableCells) {
                    color = settings_.walkableColor;
                    shouldDraw = true;
                }
            }

            if (shouldDraw) {
                // Draw cell outline using DebugRenderer
                glm::vec3 corners[4] = {
                    worldPos + glm::vec3(-halfCell, 0, -halfCell),
                    worldPos + glm::vec3( halfCell, 0, -halfCell),
                    worldPos + glm::vec3( halfCell, 0,  halfCell),
                    worldPos + glm::vec3(-halfCell, 0,  halfCell)
                };

                DebugRenderer::Get().DrawLine(corners[0], corners[1], color);
                DebugRenderer::Get().DrawLine(corners[1], corners[2], color);
                DebugRenderer::Get().DrawLine(corners[2], corners[3], color);
                DebugRenderer::Get().DrawLine(corners[3], corners[0], color);
            }
        }
    }
}

void NavigationDebug::RenderPaths(const Camera& camera) {
    float yOffset = 0.2f;

    for (const auto& [agentId, path] : activePaths_) {
        if (path.empty()) continue;

        // Draw path using DebugRenderer
        if (settings_.showPathLines && path.size() >= 2) {
            for (size_t i = 0; i < path.size() - 1; ++i) {
                glm::vec3 from = path[i];
                glm::vec3 to   = path[i + 1];
                from.y += yOffset;
                to.y   += yOffset;

                DebugRenderer::Get().DrawLine(from, to, settings_.pathColor);
            }
        }

        if (settings_.showPathNodes) {
            for (size_t i = 0; i < path.size(); ++i) {
                glm::vec3 pos = path[i];
                pos.y += yOffset;

                // First and last waypoints are larger
                float size = (i == 0 || i == path.size() - 1) ? 0.25f : 0.15f;
                DebugRenderer::Get().DrawSphere(pos, size, settings_.waypointColor, 6);
            }
        }
    }
}

void NavigationDebug::RenderSearchState(const Camera& camera, const NavigationGrid* grid) {
    if (!hasSearchState_ || !grid) return;

    float yOffset = 0.15f;

    if (settings_.showClosedList) {
        for (size_t i = 0; i < debugClosedSet_.size(); ++i) {
            if (!debugClosedSet_[i]) continue;

            GridCoord coord = grid->IndexToCoord(static_cast<int32_t>(i));
            glm::vec3 worldPos = grid->GridToWorld(coord);
            worldPos.y += yOffset;

            DebugRenderer::Get().DrawPoint(worldPos, 0.2f, settings_.closedListColor);
        }
    }

    if (settings_.showOpenList) {
        for (int32_t index : debugOpenList_) {
            GridCoord coord = grid->IndexToCoord(index);
            glm::vec3 worldPos = grid->GridToWorld(coord);
            worldPos.y += yOffset + 0.05f;

            DebugRenderer::Get().DrawPoint(worldPos, 0.2f, settings_.openListColor);
        }
    }
}

glm::vec3 NavigationDebug::GetIslandColor(int32_t islandId) const {
    if (islandId < 0) return glm::vec3(0.3f);  // Gray for unwalkable
    
    // Golden ratio for well-distributed hues
    const float goldenRatio = 0.618033988749895f;
    float hue = std::fmod(islandId * goldenRatio, 1.0f);
    
    // Convert HSL to RGB (S=0.7, L=0.5 for vivid colors)
    float s = 0.7f;
    float l = 0.5f;
    
    auto hueToRgb = [](float p, float q, float t) {
        if (t < 0) t += 1;
        if (t > 1) t -= 1;
        if (t < 1.0f/6.0f) return p + (q - p) * 6.0f * t;
        if (t < 0.5f) return q;
        if (t < 2.0f/3.0f) return p + (q - p) * (2.0f/3.0f - t) * 6.0f;
        return p;
    };
    
    float q = l < 0.5f ? l * (1 + s) : l + s - l * s;
    float p = 2 * l - q;
    
    return glm::vec3(
        hueToRgb(p, q, hue + 1.0f/3.0f),
        hueToRgb(p, q, hue),
        hueToRgb(p, q, hue - 1.0f/3.0f)
    );
}

void NavigationDebug::RenderIslands(const Camera& camera, const NavigationGrid* grid) {
    if (!grid || !grid->HasIslandData()) return;
    
    const auto& gridSettings = grid->GetSettings();
    float cellSize = gridSettings.cellSize;
    float halfCell = cellSize * 0.5f;
    float yOffset  = 0.08f;

    int maxCellsToRender = 50;
    int startX = std::max(0, gridSettings.width / 2 - maxCellsToRender / 2);
    int startZ = std::max(0, gridSettings.height / 2 - maxCellsToRender / 2);
    int endX = std::min(gridSettings.width, startX + maxCellsToRender);
    int endZ = std::min(gridSettings.height, startZ + maxCellsToRender);

    for (int32_t z = startZ; z < endZ; ++z) {
        for (int32_t x = startX; x < endX; ++x) {
            int32_t islandId = grid->GetIslandId(x, z);
            if (islandId < 0) continue;  // Skip unwalkable
            
            glm::vec3 worldPos = grid->GridToWorld(x, z);
            worldPos.y += yOffset;
            
            glm::vec3 color = GetIslandColor(islandId);
            
            glm::vec3 corners[4] = {
                worldPos + glm::vec3(-halfCell, 0, -halfCell),
                worldPos + glm::vec3( halfCell, 0, -halfCell),
                worldPos + glm::vec3( halfCell, 0,  halfCell),
                worldPos + glm::vec3(-halfCell, 0,  halfCell)
            };

        DebugRenderer::Get().DrawLine(corners[0], corners[1], color);
            DebugRenderer::Get().DrawLine(corners[1], corners[2], color);
            DebugRenderer::Get().DrawLine(corners[2], corners[3], color);
            DebugRenderer::Get().DrawLine(corners[3], corners[0], color);
        }
    }
}

void NavigationDebug::RenderNodeCostOverlay(const Camera& camera, const NavigationGrid* grid) {
    if (!grid || !grid->IsInitialized()) return;
    
    const auto& gridSettings = grid->GetSettings();
    
    glm::vec3 camPos = camera.GetPosition();
    GridCoord camCoord = grid->WorldToGrid(camPos);
    
    // Render cells around grid center if camera is outside the grid
    if (!grid->IsValidCoord(camCoord)) {
        camCoord.x = gridSettings.width / 2;
        camCoord.z = gridSettings.height / 2;
    }
    
    // Render cells in a small area around camera
    int maxCellsToRender = 30;
    float maxRenderDistance = 50.0f;
    int startX = std::max(0, camCoord.x - maxCellsToRender / 2);
    int startZ = std::max(0, camCoord.z - maxCellsToRender / 2);
    int endX = std::min(gridSettings.width, startX + maxCellsToRender);
    int endZ = std::min(gridSettings.height, startZ + maxCellsToRender);
    
    float textScale = 0.8f;
    
    for (int32_t z = startZ; z < endZ; ++z) {
        for (int32_t x = startX; x < endX; ++x) {
            const PathNode* node = grid->GetNode(x, z);
            if (!node) continue;
            
            glm::vec3 worldPos = grid->GridToWorld(x, z);
            worldPos.y += 0.2f;  // Lift text slightly above ground
            
            float dist = glm::distance(camPos, worldPos);
            if (dist > maxRenderDistance) continue;
            
            // Build text label
            char cellText[16];
            glm::vec3 textColor;
            
            if (!node->IsWalkable()) {
                snprintf(cellText, sizeof(cellText), "X");
                textColor = se::DebugColors::Red;
            } else if (node->penalty > 0.0f) {
                snprintf(cellText, sizeof(cellText), "+%.0f", node->penalty);
                textColor = se::DebugColors::Yellow;
            } else {
                // Skip rendering regular walkable cells to reduce clutter
                continue;
            }
            
            se::DebugRenderer::Get().DrawText3D(worldPos, cellText, textColor, textScale);
        }
    }
}

}  // namespace nav
}  // namespace se
