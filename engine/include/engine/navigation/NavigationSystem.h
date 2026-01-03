#pragma once

#include "engine/navigation/NavigationGrid.h"
#include "engine/navigation/AStar.h"
#include "engine/navigation/PathRequest.h"
#include "engine/navigation/NavigationDebug.h"

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <unordered_map>

namespace se {

class Scene;
class ThreadPool;

namespace nav {

// Navigation events for external systems
enum class NavigationEvent {
    PathRequested,   // Path calculation started
    PathFound,       // Path successfully found
    PathFailed,      // Path could not be found
    AgentArrived     // Agent reached destination
};

// Callback for navigation events (agent id, event type, optional path result)
using NavigationEventCallback = std::function<void(uint32_t agentId, NavigationEvent event, const PathResult* result)>;

struct NavigationStats {
    size_t totalRequests     = 0;
    size_t completedRequests = 0;
    size_t failedRequests    = 0;
    size_t cancelledRequests = 0;
    size_t cacheHits         = 0;
    size_t pendingRequests   = 0;
    float  avgComputeTimeMs  = 0.0f;
    float  lastFrameTimeMs   = 0.0f;
};

class NavigationSystem {
   public:
    explicit NavigationSystem(Scene* scene);
    ~NavigationSystem();

    NavigationSystem(const NavigationSystem&)            = delete;
    NavigationSystem& operator=(const NavigationSystem&) = delete;

    // Initialization
    void Initialize();
    void Shutdown();

    // Grid management
    void CreateGrid(const NavigationGridSettings& settings);
    NavigationGrid* GetGrid() { return grid_.get(); }
    const NavigationGrid* GetGrid() const { return grid_.get(); }
    void BakeObstacles();  // Uses physics raycasts from scene
    void RebakeObstacles();  // Force rebake (clears baked flag and rebakes)


    // Async path requests (non-blocking)
    uint32_t RequestPath(const Vector3& start, const Vector3& goal,
                         std::function<void(const PathResult&)> callback = nullptr,
                         const PathfindingSettings& settings = {});
    uint32_t RequestPath(Entity requester, const Vector3& goal,
                         std::function<void(const PathResult&)> callback = nullptr,
                         const PathfindingSettings& settings = {});
    
    // Query request status
    bool IsPathReady(uint32_t requestId) const;
    PathResult GetPathResult(uint32_t requestId);
    void CancelRequest(uint32_t requestId);

    // Sync path (blocks - use sparingly for immediate needs)
    PathResult FindPathSync(const Vector3& start, const Vector3& goal,
                            const PathfindingSettings& settings = {});

    // Dynamic obstacle management
    void RegisterObstacle(Entity entity, const Vector3& size);
    void UnregisterObstacle(Entity entity);
    void UpdateObstacles();  // Called internally to sync obstacles with grid

    // Agent registration (agents are excluded from obstacle detection)
    void RegisterAgent(Entity agent);
    void UnregisterAgent(Entity agent);
    bool IsAgent(Entity entity) const;

    // Frame update - processes async requests
    void Update(float dt);

    // Debug visualization
    NavigationDebug& GetDebug() { return debug_; }
    const NavigationDebug& GetDebug() const { return debug_; }

    // Stats
    const NavigationStats& GetStats() const { return stats_; }
    void ResetStats();

    // Configuration
    void SetMaxRequestsPerFrame(int max) { maxRequestsPerFrame_ = max; }
    int  GetMaxRequestsPerFrame() const { return maxRequestsPerFrame_; }
    
    // Event system
    uint32_t RegisterEventCallback(NavigationEventCallback callback);
    void UnregisterEventCallback(uint32_t callbackId);
    void FireEvent(uint32_t agentId, NavigationEvent event, const PathResult* result = nullptr);

   private:
    void ProcessPendingRequests();
    void ProcessRequestAsync(PathRequest request);
    void OnRequestComplete(uint32_t requestId, PathResult result);

    Scene*                          scene_;
    std::unique_ptr<NavigationGrid> grid_;
    AStar                           pathfinder_;
    NavigationDebug                 debug_;
    NavigationStats                 stats_;

    // Request management
    std::atomic<uint32_t>                        nextRequestId_{1};
    std::queue<PathRequest>                      pendingRequests_;
    std::unordered_map<uint32_t, PathResult>     completedResults_;
    std::unordered_map<uint32_t, PathRequest>    activeRequests_;
    mutable std::mutex                           requestMutex_;
    mutable std::mutex                           resultMutex_;

    // Dynamic obstacles
    struct DynamicObstacle {
        Entity  entity;
        Vector3 size;
        Vector3 lastPosition;
    };
    std::vector<DynamicObstacle> dynamicObstacles_;
    std::mutex                   obstacleMutex_;

    // Configuration
    int  maxRequestsPerFrame_ = 10;  // Limit per-frame to maintain FPS
    bool initialized_         = false;

    // Registered agents (excluded from obstacle detection)
    std::vector<Entity> registeredAgents_;
    
    // Event callbacks
    std::unordered_map<uint32_t, NavigationEventCallback> eventCallbacks_;
    std::atomic<uint32_t> nextCallbackId_{1};
};

}  // namespace nav
}  // namespace se
