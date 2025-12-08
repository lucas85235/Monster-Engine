#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <queue>
#include <memory>

#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <btBulletCollisionCommon.h>

#include "engine/ecs/Entity.h"
#include "engine/Camera.h"
#include "engine/core/ThreadPool.h"

#include <glm.hpp>
#include <gtc/quaternion.hpp>

namespace se {

class Scene;
class PhysicsDebugDraw;
struct RigidbodyData;

struct CachedTransform {
    glm::vec3 position;
    glm::quat rotation;
};

class PhysicsSystem {
public:
    PhysicsSystem(Scene* scene);
    ~PhysicsSystem();

    void Initialize();
    void Shutdown();

    void Update(float dt);
    
    btRigidBody* AddRigidBody(Entity entity, const RigidbodyData& data);
    void RemoveRigidBody(btRigidBody* body);

    void RenderDebug(const Camera& camera);

    bool Raycast(const glm::vec3& start, const glm::vec3& end, glm::vec3& hitPoint, glm::vec3& hitNormal, btRigidBody* ignoredBody = nullptr);
    btRigidBody* RaycastHitBody(const glm::vec3& start, const glm::vec3& end, glm::vec3& hitPoint, btRigidBody* ignoredBody = nullptr);

    bool RaycastSync(const glm::vec3& start, const glm::vec3& end, glm::vec3& hitPoint, glm::vec3& hitNormal, btRigidBody* ignoredBody = nullptr);
    btRigidBody* RaycastHitBodySync(const glm::vec3& start, const glm::vec3& end, glm::vec3& hitPoint, btRigidBody* ignoredBody = nullptr);

    btDiscreteDynamicsWorld* GetDynamicsWorld() { return dynamics_world_; }
    PhysicsDebugDraw* GetDebugDrawer() { return debug_drawer_; }
    
    void UpdateDebugDraw(float dt);
    
    std::mutex& GetMutex() { return physics_mutex_; }
    float GetLastPhysicsExecutionTime() const { return last_physics_execution_time_; }
    size_t GetThreadPoolSize() const { return thread_pool_ ? thread_pool_->GetThreadCount() : 0; }

private:
    void PhysicsLoop();
    void ProcessPendingCommands();
    void RemoveBodyInternal(btRigidBody* body);
    void SyncTransformsToCache();
    void SyncTransformsToCacheParallel();

    Scene* scene_;
    
    // Thread pool for parallel physics operations
    std::unique_ptr<ThreadPool> thread_pool_;
    
    // Bullet Physics
    btDiscreteDynamicsWorld*             dynamics_world_ = nullptr;
    btDefaultCollisionConfiguration*     collision_configuration_ = nullptr;
    btCollisionDispatcher*               dispatcher_ = nullptr;
    btBroadphaseInterface*               overlapping_pair_cache_broadphase_interface_ = nullptr;
    btSequentialImpulseConstraintSolver* solver_ = nullptr;
    PhysicsDebugDraw*                    debug_drawer_ = nullptr;

    // Physics thread
    std::thread         physics_thread_;
    std::mutex          physics_mutex_;
    std::atomic<bool>   running_ = false;
    std::atomic<float>  last_physics_execution_time_ = 0.0f;

    // Deferred body operations
    struct PendingAddBody {
        Entity entity;
        btRigidBody* body;
        uint16_t collision_group;
        uint16_t collision_mask;
    };
    std::queue<PendingAddBody> pending_add_bodies_;
    std::queue<btRigidBody*> pending_remove_bodies_;
    std::mutex command_queue_mutex_;

    // Body tracking
    struct BodyEntry {
        Entity entity;
        btRigidBody* body;
    };
    std::vector<BodyEntry> bodies_;
    
    // Transform cache - accessed in parallel
    struct TransformCacheEntry {
        Entity entity;
        CachedTransform transform;
    };
    std::vector<TransformCacheEntry> transform_cache_;
    std::mutex transform_cache_mutex_;
};

} // namespace se
