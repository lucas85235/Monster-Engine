#pragma once

#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <queue>
#include <functional>

#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <btBulletCollisionCommon.h>

#include "engine/ecs/Entity.h"
#include "engine/Camera.h"

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

    btDiscreteDynamicsWorld* GetDynamicsWorld() { return dynamics_world_; }
    
    // Thread-safe access to lock the world if needed manually
    std::mutex& GetMutex() { return physics_mutex_; }

private:
    void PhysicsLoop();

    Scene* scene_;
    
    // Bullet Physics
    btDiscreteDynamicsWorld*             dynamics_world_ = nullptr;
    btDefaultCollisionConfiguration*     collision_configuration_ = nullptr;
    btCollisionDispatcher*               dispatcher_ = nullptr;
    btBroadphaseInterface*               overlapping_pair_cache_broadphase_interface_ = nullptr;
    btSequentialImpulseConstraintSolver* solver_ = nullptr;
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
    
    // Command Queue for deferred operations if needed (currently using mutex for immediate access)
    // std::queue<std::function<void()>> command_queue_;

    struct BodyEntry {
        Entity entity;
        btRigidBody* body;
    };
    std::vector<BodyEntry> bodies_;
};

} // namespace se
