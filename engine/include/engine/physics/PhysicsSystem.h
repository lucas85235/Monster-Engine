#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <queue>
#include <functional>
#include <memory>

#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <btBulletCollisionCommon.h>

#include "engine/ecs/Entity.h"
#include "engine/Camera.h"

class btConstraintSolverPoolMt;
class btITaskScheduler;

namespace se {

class Scene;
class PhysicsDebugDraw;
struct RigidbodyData;

class PhysicsSystem {
public:
    PhysicsSystem(Scene* scene);
    ~PhysicsSystem();

    void Initialize();
    void Shutdown();

    // Called from Main Thread
    void Update(float dt);
    
    // Called from Main Thread (usually via RigidbodyComponent)
    btRigidBody* AddRigidBody(Entity entity, const RigidbodyData& data);
    void RemoveRigidBody(btRigidBody* body);

    void RenderDebug(const Camera& camera);

    // Raycast
    bool Raycast(const glm::vec3& start, const glm::vec3& end, glm::vec3& hitPoint, glm::vec3& hitNormal, btRigidBody* ignoredBody = nullptr);
    
    // Raycast that also returns the hit rigidbody (for grabbing objects)
    btRigidBody* RaycastHitBody(const glm::vec3& start, const glm::vec3& end, glm::vec3& hitPoint, btRigidBody* ignoredBody = nullptr);

    btDiscreteDynamicsWorld* GetDynamicsWorld() { return dynamics_world_; }
    PhysicsDebugDraw* GetDebugDrawer() { return debug_drawer_; }
    
    // Update debug drawing (call from main thread after Update)
    void UpdateDebugDraw(float dt);
    
    // Thread-safe access to lock the world if needed manually
    std::mutex& GetMutex() { return physics_mutex_; }

private:
    void PhysicsLoop();

    Scene* scene_;
    
    // Bullet Physics
    // Using btDiscreteDynamicsWorldMt for multithreading
    btDiscreteDynamicsWorld*             dynamics_world_ = nullptr;
    btDefaultCollisionConfiguration*     collision_configuration_ = nullptr;
    btCollisionDispatcher*               dispatcher_ = nullptr;
    btBroadphaseInterface*               overlapping_pair_cache_broadphase_interface_ = nullptr;
    btSequentialImpulseConstraintSolver* solver_ = nullptr;
    
    // MT specific
    btConstraintSolverPoolMt*            solver_pool_ = nullptr;
    btITaskScheduler*                    task_scheduler_ = nullptr;

    PhysicsDebugDraw*                    debug_drawer_ = nullptr;

    // Threading
    std::thread         physics_thread_;
    std::mutex          physics_mutex_;
    std::atomic<bool>   running_ = false;
    std::atomic<float>  accumulated_time_ = 0.0f;
    std::atomic<float>  last_physics_execution_time_ = 0.0f;

public:
    float GetLastPhysicsExecutionTime() const { return last_physics_execution_time_; }

private:
    void ProcessPendingCommands();
    void RemoveBodyInternal(btRigidBody* body);

    // Deferred command queue for thread-safe body operations
    struct PendingAddBody {
        Entity entity;
        btRigidBody* body;
        uint16_t collision_group;
        uint16_t collision_mask;
    };
    std::queue<PendingAddBody> pending_add_bodies_;
    std::queue<btRigidBody*> pending_remove_bodies_;
    std::mutex command_queue_mutex_;

    struct BodyEntry {
        Entity entity;
        btRigidBody* body;
    };
    std::vector<BodyEntry> bodies_;
};

} // namespace se
