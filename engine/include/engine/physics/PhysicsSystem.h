#pragma once

#include <BulletCollision/CollisionDispatch/btCollisionDispatcherMt.h>
#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorldMt.h>
#include <LinearMath/btThreads.h>
#include <btBulletCollisionCommon.h>

#include <functional>
#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include <mutex>
#include <queue>
#include <vector>

#include "engine/Camera.h"
#include "engine/ecs/Entity.h"

namespace se {

// Callback type for physics pre-tick (called before each Bullet substep)
// Parameter: fixedTimeStep (1/60 by default)
using PhysicsPreTickCallback = std::function<void(float)>;

class Scene;
class PhysicsDebugDraw;
struct RigidbodyData;

struct CachedTransform {
    glm::vec3 position;
    glm::quat rotation;
};

struct RaycastRequest {
    glm::vec3 start;
    glm::vec3 end;
};

struct RaycastResult {
    bool      hit = false;
    glm::vec3 hitPoint{0.0f};
    glm::vec3 hitNormal{0.0f, 1.0f, 0.0f};
};

// Physics configuration for tuning
struct PhysicsConfig {
    // maxSubSteps: How many fixed timestep iterations Bullet can do per frame
    // With VSync off at high framerates (300+ FPS), dt could be very small (~3ms)
    // Bullet internally accumulates time and runs fixedTimeStep iterations
    // Value of 0 = variable timestep (non-deterministic, NOT recommended)
    // Higher values = catch up faster after frame spikes but cost more CPU
    int   maxSubSteps   = 10;  // Increased to handle high framerates deterministically
    float fixedTimeStep = 1.0f / 60.0f;  // 60 Hz physics simulation
    float gravity       = -9.81f;

    // Deactivation thresholds
    float linearSleepThreshold  = 0.8f;  // Default is 0.8
    float angularSleepThreshold = 1.0f;  // Default is 1.0
    float deactivationTime      = 2.0f;  // Seconds before sleeping

    // Parallel thresholds
    size_t parallelThreshold = 50;  // Use parallel for N+ bodies

    // Solver iterations (lower = faster but less accurate)
    int solverIterations = 4;  // Default is 10
};

class PhysicsSystem {
   public:
    PhysicsSystem(Scene* scene);
    ~PhysicsSystem();

    void Initialize();
    void Initialize(const PhysicsConfig& config);
    void Shutdown();

    void Update(float dt);

    btRigidBody* AddRigidBody(Entity entity, const RigidbodyData& data);
    void         RemoveRigidBody(btRigidBody* body);

    void RenderDebug(const Camera& camera);

    bool         Raycast(const glm::vec3& start, const glm::vec3& end, glm::vec3& hitPoint,
                         glm::vec3& hitNormal, btRigidBody* ignoredBody = nullptr);
    btRigidBody* RaycastHitBody(const glm::vec3& start, const glm::vec3& end, glm::vec3& hitPoint,
                                btRigidBody* ignoredBody = nullptr);

    bool         RaycastSync(const glm::vec3& start, const glm::vec3& end, glm::vec3& hitPoint,
                             glm::vec3& hitNormal, btRigidBody* ignoredBody = nullptr);
    btRigidBody* RaycastHitBodySync(const glm::vec3& start, const glm::vec3& end,
                                    glm::vec3& hitPoint, btRigidBody* ignoredBody = nullptr);

    // Batch raycast - single mutex lock for multiple rays (more efficient for navmesh baking)
    void RaycastBatch(const std::vector<RaycastRequest>& requests,
                      std::vector<RaycastResult>& results);

    // AABB overlap check - returns true if any physics body overlaps the box
    bool OverlapAABB(const glm::vec3& min, const glm::vec3& max);

    btDiscreteDynamicsWorld* GetDynamicsWorld() {
        return dynamics_world_;
    }
    PhysicsDebugDraw* GetDebugDrawer() {
        return debug_drawer_;
    }

    void UpdateDebugDraw(float dt);

    // Force processing of any pending add/remove body commands
    // Call this before raycasting if bodies were recently added
    void FlushPendingBodies() { ProcessPendingCommands(); }

    std::mutex& GetMutex() {
        return physics_mutex_;
    }
    float GetLastPhysicsExecutionTime() const {
        return last_physics_execution_time_;
    }
    size_t GetThreadPoolSize() const {
        return task_scheduler_ ? static_cast<size_t>(task_scheduler_->getNumThreads()) : 0;
    }
    size_t GetActiveBodyCount() const {
        return bodies_.size();
    }
    size_t GetSleepingBodyCount() const;

    const PhysicsConfig& GetConfig() const {
        return config_;
    }
    bool IsIdle() const {
        return all_bodies_sleeping_;
    }

    // Set callback to be called before each physics substep
    // Use this to sync game logic with physics timing
    void SetPreTickCallback(PhysicsPreTickCallback callback) {
        pre_tick_callback_ = std::move(callback);
    }

   private:
    static void BulletPreTickCallback(btDynamicsWorld* world, btScalar timeStep);
    
    void ProcessPendingCommands();
    void RemoveBodyInternal(btRigidBody* body);
    void ConfigureBodyDeactivation(btRigidBody* body);

    Scene*        scene_;
    PhysicsConfig config_;

    btDiscreteDynamicsWorld*         dynamics_world_                              = nullptr;
    btDefaultCollisionConfiguration* collision_configuration_                     = nullptr;
    btCollisionDispatcher*           dispatcher_                                  = nullptr;
    btBroadphaseInterface*           overlapping_pair_cache_broadphase_interface_ = nullptr;
    btConstraintSolver*              solver_                                      = nullptr;
    btConstraintSolverPoolMt*        solver_pool_                                 = nullptr;
    btITaskScheduler*                task_scheduler_                              = nullptr;
    PhysicsDebugDraw*                debug_drawer_                                = nullptr;

    std::mutex physics_mutex_;
    bool       running_                     = false;
    float      last_physics_execution_time_ = 0.0f;
    bool       all_bodies_sleeping_         = false;
    
    PhysicsPreTickCallback pre_tick_callback_;

    struct PendingAddBody {
        Entity       entity;
        btRigidBody* body;
        uint16_t     collision_group;
        uint16_t     collision_mask;
        bool         usesCachedShape;  // If true, don't delete shape on removal
    };
    std::queue<PendingAddBody> pending_add_bodies_;
    std::queue<btRigidBody*>   pending_remove_bodies_;
    std::mutex                 command_queue_mutex_;

    struct BodyEntry {
        Entity       entity;
        btRigidBody* body;
        bool         usesCachedShape;
    };
    std::vector<BodyEntry> bodies_;

    struct TransformCacheEntry {
        Entity          entity;
        CachedTransform transform;
    };
    std::vector<TransformCacheEntry> transform_cache_;
    std::mutex                       transform_cache_mutex_;
};

}  // namespace se
