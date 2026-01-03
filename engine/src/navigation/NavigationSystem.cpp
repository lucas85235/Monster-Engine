#include "engine/navigation/NavigationSystem.h"
#include "engine/navigation/PathfindingAgentComponent.h"
#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/physics/PhysicsSystem.h"
#include "engine/core/ThreadPool.h"
#include "engine/Log.h"

#include <algorithm>
#include <chrono>


namespace se {
namespace nav {

// Global thread pool for navigation (shared across all scenes)
static std::unique_ptr<ThreadPool> s_navThreadPool;

static ThreadPool& GetNavThreadPool() {
    if (!s_navThreadPool) {
        size_t threadCount = std::max(2u, std::thread::hardware_concurrency() / 2);
        s_navThreadPool = std::make_unique<ThreadPool>(threadCount);
        SE_LOG_INFO("[NavigationSystem] Created navigation thread pool with {} threads", threadCount);
    }
    return *s_navThreadPool;
}

NavigationSystem::NavigationSystem(Scene* scene)
    : scene_(scene) {
}

NavigationSystem::~NavigationSystem() {
    Shutdown();
}

void NavigationSystem::Initialize() {
    if (initialized_) {
        SE_LOG_WARN("[NavigationSystem] Already initialized");
        return;
    }

    initialized_ = true;
    SE_LOG_INFO("[NavigationSystem] Initialized");
}

void NavigationSystem::Shutdown() {
    if (!initialized_) return;

    {
        std::lock_guard<std::mutex> lock(requestMutex_);
        while (!pendingRequests_.empty()) {
            pendingRequests_.pop();
        }
        activeRequests_.clear();
    }

    {
        std::lock_guard<std::mutex> lock(resultMutex_);
        completedResults_.clear();
    }

    {
        std::lock_guard<std::mutex> lock(obstacleMutex_);
        dynamicObstacles_.clear();
    }

    if (grid_) {
        grid_->Shutdown();
        grid_.reset();
    }

    initialized_ = false;
    SE_LOG_INFO("[NavigationSystem] Shutdown");
}

void NavigationSystem::CreateGrid(const NavigationGridSettings& settings) {
    grid_ = std::make_unique<NavigationGrid>();
    grid_->Initialize(settings);
}

void NavigationSystem::BakeObstacles() {
    if (!grid_) {
        SE_LOG_ERROR("[NavigationSystem] Cannot bake obstacles: no grid created");
        return;
    }

    if (!scene_ || !scene_->HasPhysics()) {
        SE_LOG_WARN("[NavigationSystem] Cannot bake obstacles: no physics system");
        return;
    }

    // CRITICAL: Flush pending body adds before raycasting!
    // Bodies are added via queue and not available until ProcessPendingCommands runs
    scene_->GetPhysicsSystem()->FlushPendingBodies();
    
    grid_->BakeObstacles(scene_->GetPhysicsSystem());
}

void NavigationSystem::RebakeObstacles() {
    if (!grid_) {
        SE_LOG_ERROR("[NavigationSystem] Cannot rebake obstacles: no grid created");
        return;
    }

    if (!scene_ || !scene_->HasPhysics()) {
        SE_LOG_WARN("[NavigationSystem] Cannot rebake obstacles: no physics system");
        return;
    }

    scene_->GetPhysicsSystem()->FlushPendingBodies();
    grid_->RebakeObstacles(scene_->GetPhysicsSystem());
}


uint32_t NavigationSystem::RequestPath(const Vector3& start, const Vector3& goal,
                                        std::function<void(const PathResult&)> callback,
                                        const PathfindingSettings& settings) {

    PathRequest request;
    request.requestId  = nextRequestId_++;
    request.start      = start;
    request.goal       = goal;
    request.settings   = settings;
    request.onComplete = std::move(callback);
    request.status     = PathRequestStatus::Pending;

    {
        std::lock_guard<std::mutex> lock(requestMutex_);
        pendingRequests_.push(request);
        stats_.totalRequests++;
        stats_.pendingRequests = pendingRequests_.size();
    }

    SE_LOG_DEBUG("[NavigationSystem] Path request {} queued: ({:.1f},{:.1f},{:.1f}) -> ({:.1f},{:.1f},{:.1f})",
                 request.requestId, start.x, start.y, start.z, goal.x, goal.y, goal.z);

    return request.requestId;
}

uint32_t NavigationSystem::RequestPath(Entity requester, const Vector3& goal,
                                        std::function<void(const PathResult&)> callback,
                                        const PathfindingSettings& settings) {
    if (!requester.IsValid() || !requester.HasComponent<TransformComponent>()) {
        SE_LOG_WARN("[NavigationSystem] Invalid requester entity");
        return 0;
    }

    Vector3 start = requester.GetComponent<TransformComponent>().Position;
    
    PathRequest request;
    request.requestId  = nextRequestId_++;
    request.requester  = requester;
    request.start      = start;
    request.goal       = goal;
    request.settings   = settings;
    request.onComplete = std::move(callback);
    request.status     = PathRequestStatus::Pending;

    {
        std::lock_guard<std::mutex> lock(requestMutex_);
        pendingRequests_.push(request);
        stats_.totalRequests++;
        stats_.pendingRequests = pendingRequests_.size();
    }

    return request.requestId;
}

bool NavigationSystem::IsPathReady(uint32_t requestId) const {
    std::lock_guard<std::mutex> lock(resultMutex_);
    return completedResults_.find(requestId) != completedResults_.end();
}

PathResult NavigationSystem::GetPathResult(uint32_t requestId) {
    std::lock_guard<std::mutex> lock(resultMutex_);
    auto it = completedResults_.find(requestId);
    if (it != completedResults_.end()) {
        PathResult result = std::move(it->second);
        completedResults_.erase(it);
        return result;
    }
    return PathResult{};
}

void NavigationSystem::CancelRequest(uint32_t requestId) {
    {
        std::lock_guard<std::mutex> lock(requestMutex_);
        auto it = activeRequests_.find(requestId);
        if (it != activeRequests_.end()) {
            it->second.status = PathRequestStatus::Cancelled;
            stats_.cancelledRequests++;
        }
    }
}

PathResult NavigationSystem::FindPathSync(const Vector3& start, const Vector3& goal,
                                          const PathfindingSettings& settings) {
    if (!grid_) {
        SE_LOG_ERROR("[NavigationSystem] Cannot find path: no grid");
        return PathResult{};
    }

    return pathfinder_.FindPath(*grid_, start, goal, settings);
}

void NavigationSystem::RegisterObstacle(Entity entity, const Vector3& size) {
    if (!entity.IsValid()) return;

    std::lock_guard<std::mutex> lock(obstacleMutex_);
    
    for (auto& obs : dynamicObstacles_) {
        if (obs.entity == entity) {
            obs.size = size;
            return;
        }
    }

    DynamicObstacle obstacle;
    obstacle.entity = entity;
    obstacle.size   = size;
    if (entity.HasComponent<TransformComponent>()) {
        obstacle.lastPosition = entity.GetComponent<TransformComponent>().Position;
    }
    dynamicObstacles_.push_back(obstacle);

    SE_LOG_DEBUG("[NavigationSystem] Registered dynamic obstacle: entity {}", entity.GetID());
}

void NavigationSystem::UnregisterObstacle(Entity entity) {
    std::lock_guard<std::mutex> lock(obstacleMutex_);
    
    auto it = std::remove_if(dynamicObstacles_.begin(), dynamicObstacles_.end(),
        [&entity](const DynamicObstacle& obs) { return obs.entity == entity; });
    
    if (it != dynamicObstacles_.end()) {
        dynamicObstacles_.erase(it, dynamicObstacles_.end());
        SE_LOG_DEBUG("[NavigationSystem] Unregistered dynamic obstacle: entity {}", entity.GetID());
    }
}

void NavigationSystem::RegisterAgent(Entity agent) {
    if (!agent.IsValid()) return;
    
    auto it = std::find(registeredAgents_.begin(), registeredAgents_.end(), agent);
    if (it == registeredAgents_.end()) {
        registeredAgents_.push_back(agent);
        SE_LOG_DEBUG("[NavigationSystem] Registered agent: entity {}", agent.GetID());
    }
}

void NavigationSystem::UnregisterAgent(Entity agent) {
    auto it = std::find(registeredAgents_.begin(), registeredAgents_.end(), agent);
    if (it != registeredAgents_.end()) {
        registeredAgents_.erase(it);
        SE_LOG_DEBUG("[NavigationSystem] Unregistered agent: entity {}", agent.GetID());
    }
}

bool NavigationSystem::IsAgent(Entity entity) const {
    return std::find(registeredAgents_.begin(), registeredAgents_.end(), entity) != registeredAgents_.end();
}


void NavigationSystem::UpdateObstacles() {
    if (!grid_) return;

    std::lock_guard<std::mutex> lock(obstacleMutex_);

    grid_->ClearDynamicObstacles();

    for (auto& obstacle : dynamicObstacles_) {
        if (!obstacle.entity.IsValid()) continue;
        if (!obstacle.entity.HasComponent<TransformComponent>()) continue;

        Vector3 pos = obstacle.entity.GetComponent<TransformComponent>().Position;
        Vector3 halfSize = obstacle.size * 0.5f;

        GridCoord minCoord = grid_->WorldToGrid(pos - halfSize);
        GridCoord maxCoord = grid_->WorldToGrid(pos + halfSize);

        for (int32_t z = minCoord.z; z <= maxCoord.z; ++z) {
            for (int32_t x = minCoord.x; x <= maxCoord.x; ++x) {
                PathNode* node = grid_->GetNode(x, z);
                if (node) {
                    node->flags = node->flags | NodeFlags::Obstacle | NodeFlags::Dynamic;
                }
            }
        }

        obstacle.lastPosition = pos;
    }
}

void NavigationSystem::Update(float dt) {
    if (!initialized_) return;

    auto frameStart = std::chrono::high_resolution_clock::now();

    UpdateObstacles();

    ProcessPendingRequests();

    auto frameEnd = std::chrono::high_resolution_clock::now();
    stats_.lastFrameTimeMs = std::chrono::duration<float, std::milli>(frameEnd - frameStart).count();
}

void NavigationSystem::ProcessPendingRequests() {
    if (!grid_) return;

    int processedThisFrame = 0;

    while (processedThisFrame < maxRequestsPerFrame_) {
        PathRequest request;
        
        {
            std::lock_guard<std::mutex> lock(requestMutex_);
            if (pendingRequests_.empty()) break;

            request = std::move(pendingRequests_.front());
            pendingRequests_.pop();
            stats_.pendingRequests = pendingRequests_.size();
            
            activeRequests_[request.requestId] = request;
        }

        GetNavThreadPool().Submit([this, request]() mutable {
            ProcessRequestAsync(request);
        });

        processedThisFrame++;
    }
}

void NavigationSystem::ProcessRequestAsync(PathRequest request) {
    request.status = PathRequestStatus::Computing;

    PathResult result = pathfinder_.FindPath(*grid_, request.start, request.goal, request.settings);

    {
        std::lock_guard<std::mutex> lock(requestMutex_);
        auto it = activeRequests_.find(request.requestId);
        if (it != activeRequests_.end() && it->second.status == PathRequestStatus::Cancelled) {
            activeRequests_.erase(it);
            return;
        }
        activeRequests_.erase(request.requestId);
    }

    OnRequestComplete(request.requestId, std::move(result));

    if (request.onComplete) {
        std::lock_guard<std::mutex> lock(resultMutex_);
        auto it = completedResults_.find(request.requestId);
        if (it != completedResults_.end()) {
            request.onComplete(it->second);
        }
    }
}

void NavigationSystem::OnRequestComplete(uint32_t requestId, PathResult result) {
    std::lock_guard<std::mutex> lock(resultMutex_);
    
    if (result.success) {
        stats_.completedRequests++;
        debug_.AddActivePath(requestId, result.path);
        FireEvent(requestId, NavigationEvent::PathFound, &result);
    } else {
        stats_.failedRequests++;
        FireEvent(requestId, NavigationEvent::PathFailed, &result);
    }

    float totalTime = stats_.avgComputeTimeMs * (stats_.completedRequests + stats_.failedRequests - 1);
    stats_.avgComputeTimeMs = (totalTime + result.computeTimeMs) / 
                              (stats_.completedRequests + stats_.failedRequests);

    completedResults_[requestId] = std::move(result);
}

void NavigationSystem::ResetStats() {
    stats_ = NavigationStats{};
}

uint32_t NavigationSystem::RegisterEventCallback(NavigationEventCallback callback) {
    uint32_t id = nextCallbackId_++;
    eventCallbacks_[id] = std::move(callback);
    SE_LOG_DEBUG("[NavigationSystem] Registered event callback {}", id);
    return id;
}

void NavigationSystem::UnregisterEventCallback(uint32_t callbackId) {
    auto it = eventCallbacks_.find(callbackId);
    if (it != eventCallbacks_.end()) {
        eventCallbacks_.erase(it);
        SE_LOG_DEBUG("[NavigationSystem] Unregistered event callback {}", callbackId);
    }
}

void NavigationSystem::FireEvent(uint32_t agentId, NavigationEvent event, const PathResult* result) {
    for (const auto& [id, callback] : eventCallbacks_) {
        if (callback) {
            callback(agentId, event, result);
        }
    }
}

}  // namespace nav
}  // namespace se
