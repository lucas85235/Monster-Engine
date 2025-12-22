#include "engine/physics/PhysicsSystem.h"
#include "engine/physics/ShapeCache.h"

#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorldMt.h>
#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolverMt.h>
#include <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <BulletCollision/CollisionDispatch/btCollisionDispatcherMt.h>

#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/physics/Collider.h"
#include "engine/physics/RigidbodyComponent.h"
#include "engine/physics/PhysicsDebugDraw.h"
#include "engine/Log.h"

#include <chrono>
#include <algorithm>

#include "engine/core/PerformanceProfiler.h"

namespace se {
PhysicsSystem::PhysicsSystem(Scene* scene) : scene_(scene) {}

PhysicsSystem::~PhysicsSystem() {
    Shutdown();
}

void PhysicsSystem::Initialize() {
    Initialize(PhysicsConfig{});
}

void PhysicsSystem::Initialize(const PhysicsConfig& config) {
    config_ = config;

    int num_threads = static_cast<int>(std::thread::hardware_concurrency());
    if (num_threads == 0) num_threads = 4;

    SE_LOG_INFO("Initializing PhysicsSystem with {} hardware threads...", num_threads);

    // Initialize Bullet's internal task scheduler for parallel physics
    task_scheduler_ = btCreateDefaultTaskScheduler();
    if (task_scheduler_) {
        task_scheduler_->setNumThreads(static_cast<int>(num_threads));
        btSetTaskScheduler(task_scheduler_);
        SE_LOG_INFO("Bullet task scheduler created with {} threads",
                    task_scheduler_->getNumThreads());
    } else {
        SE_LOG_WARN("Failed to create Bullet task scheduler, falling back to single-threaded");
    }

    btDefaultCollisionConstructionInfo cci;
    cci.m_defaultMaxPersistentManifoldPoolSize = 1048576;
    cci.m_defaultMaxCollisionAlgorithmPoolSize = 1048576;
    collision_configuration_                   = new btDefaultCollisionConfiguration(cci);

    // Use multi-threaded collision dispatcher
    dispatcher_ = new btCollisionDispatcherMt(collision_configuration_, 40);
    overlapping_pair_cache_broadphase_interface_ = new btDbvtBroadphase();

    // Create solver pool with one solver per thread for parallel constraint solving
    solver_pool_ = new btConstraintSolverPoolMt(static_cast<int>(num_threads));

    // Create a single MT solver for large islands
    solver_ = new btSequentialImpulseConstraintSolverMt();

    // Use multi-threaded dynamics world
    dynamics_world_ = new btDiscreteDynamicsWorldMt(
        dispatcher_,
        overlapping_pair_cache_broadphase_interface_,
        solver_pool_,
        solver_,
        collision_configuration_
        );

    dynamics_world_->setGravity(btVector3(0.0f, config_.gravity, 0.0f));

    // Optimize solver
    dynamics_world_->getSolverInfo().m_numIterations = config_.solverIterations;
    dynamics_world_->getSolverInfo().m_splitImpulse  = true;

    debug_drawer_ = new PhysicsDebugDraw();
    dynamics_world_->setDebugDrawer(debug_drawer_);

    SE_LOG_INFO(
        "Physics initialized (MULTITHREADED): {} Bullet worker threads, {} solver iters, {} max substeps",
        task_scheduler_ ? task_scheduler_->getNumThreads() : 1, config_.solverIterations,
        config_.maxSubSteps);

    running_ = true;
    // NOTE: No separate physics thread! Bullet MT manages its own worker threads internally.
    // stepSimulation is called from Update() on the main thread.
}

void PhysicsSystem::Shutdown() {
    running_ = false;

    if (dynamics_world_) {
        for (auto& entry : bodies_) {
            if (entry.body) {
                dynamics_world_->removeRigidBody(entry.body);
                delete entry.body->getMotionState();

                // Only delete shape if not from cache
                if (!entry.usesCachedShape) {
                    btCollisionShape* shape = entry.body->getCollisionShape();
                    if (shape) {
                        if (shape->isCompound()) {
                            btCompoundShape* compound   = static_cast<btCompoundShape*>(shape);
                            int              childCount = compound->getNumChildShapes();
                            for (int i = childCount - 1; i >= 0; i--) {
                                // Don't delete cached child shapes
                            }
                        }
                        delete shape;
                    }
                }

                delete entry.body;
            }
        }
        bodies_.clear();

        delete dynamics_world_;
        delete solver_;
        delete solver_pool_;
        delete overlapping_pair_cache_broadphase_interface_;
        delete dispatcher_;
        delete collision_configuration_;
        delete debug_drawer_;

        // Reset task scheduler
        if (task_scheduler_) {
            btSetTaskScheduler(nullptr);
            delete task_scheduler_;
            task_scheduler_ = nullptr;
        }

        dynamics_world_ = nullptr;
    }
}

// PhysicsLoop removed - Bullet MT runs on main thread and manages its own workers


void PhysicsSystem::ProcessPendingCommands() {
    std::lock_guard<std::mutex> lock(command_queue_mutex_);

    while (!pending_remove_bodies_.empty()) {
        btRigidBody* body = pending_remove_bodies_.front();
        pending_remove_bodies_.pop();
        RemoveBodyInternal(body);
    }

    while (!pending_add_bodies_.empty()) {
        auto& pending = pending_add_bodies_.front();
        dynamics_world_->addRigidBody(pending.body, pending.collision_group,
                                      pending.collision_mask);
        ConfigureBodyDeactivation(pending.body);
        bodies_.push_back({pending.entity, pending.body, pending.usesCachedShape});
        pending_add_bodies_.pop();
    }
}

void PhysicsSystem::ConfigureBodyDeactivation(btRigidBody* body) {
    body->setSleepingThresholds(config_.linearSleepThreshold, config_.angularSleepThreshold);
    body->setDeactivationTime(config_.deactivationTime);
}

size_t PhysicsSystem::GetSleepingBodyCount() const {
    // NOTE: This is called from main thread, bodies_ is accessed from physics thread
    // We return a cached value instead of iterating to avoid race conditions
    size_t total = bodies_.size();
    if (all_bodies_sleeping_) return total;

    // Approximate count - not 100% accurate but thread-safe
    size_t count = 0;
    for (size_t i = 0; i < total; ++i) {
        if (i < bodies_.size()) {
            const auto& entry = bodies_[i];
            if (entry.body && !entry.body->isActive()) {
                ++count;
            }
        }
    }
    return count;
}

void PhysicsSystem::Update(float dt) {
    if (!running_ || !dynamics_world_) return;

    using Clock = std::chrono::high_resolution_clock;

    // Process pending add/remove commands BEFORE simulation
    ProcessPendingCommands();

    // Run physics simulation (Bullet MT handles parallelism internally)
    auto start_physics = Clock::now();
    dynamics_world_->stepSimulation(dt, config_.maxSubSteps, config_.fixedTimeStep);
    auto end_physics = Clock::now();

    std::chrono::duration<float, std::milli> physics_duration = end_physics - start_physics;
    last_physics_execution_time_                              = physics_duration.count();

    // Count active bodies
    size_t active_body_count = 0;
    for (const auto& entry : bodies_) {
        if (entry.body && entry.body->isActive()) {
            ++active_body_count;
        }
    }

    // Update idle state
    all_bodies_sleeping_ = (active_body_count == 0) && !bodies_.empty();

    // Update profiler metrics
    {
        size_t sleeping = bodies_.size() - active_body_count;
        PerformanceProfiler::Get().SetPhysicsMetrics(
            active_body_count, sleeping, bodies_.size(),
            all_bodies_sleeping_, last_physics_execution_time_);
        PerformanceProfiler::Get().SetThreadPoolInfo(
            0, task_scheduler_ ? task_scheduler_->getNumThreads() : 0);
    }

    // Sync transforms from Bullet to ECS - only if something is moving
    if (active_body_count > 0) {
        for (auto& entry : bodies_) {
            if (!entry.body || !entry.entity.IsValid()) continue;
            if (!entry.body->isActive()) continue; // Skip sleeping bodies

            btTransform         trans  = entry.body->getInterpolationWorldTransform();
            const btVector3&    origin = trans.getOrigin();
            const btQuaternion& rot    = trans.getRotation();

            auto& transform_component    = entry.entity.GetComponent<TransformComponent>();
            transform_component.Position = glm::vec3(
                static_cast<float>(origin.getX()),
                static_cast<float>(origin.getY()),
                static_cast<float>(origin.getZ())
                );

            glm::quat q(
                static_cast<float>(rot.w()),
                static_cast<float>(rot.x()),
                static_cast<float>(rot.y()),
                static_cast<float>(rot.z())
                );
            q                              = glm::normalize(q);
            transform_component.Rotation.x = glm::degrees(glm::pitch(q));
            transform_component.Rotation.y = glm::degrees(glm::yaw(q));
            transform_component.Rotation.z = glm::degrees(glm::roll(q));

            transform_component.MarkDirty();
        }
    }
}

btRigidBody* PhysicsSystem::AddRigidBody(Entity entity, const RigidbodyData& data) {
    if (!running_) return nullptr;

    auto&     transform_component = entity.GetComponent<TransformComponent>();
    btVector3 scale(transform_component.Scale.x, transform_component.Scale.y,
                    transform_component.Scale.z);

    btCollisionShape* final_shape = nullptr;
    btCollisionShape* child_shape = nullptr;
    btVector3         offset(0, 0, 0);
    uint16_t          collision_group   = 0x0001;
    uint16_t          collision_mask    = 0xFFFF;
    bool              is_trigger        = false;
    bool              uses_cached_shape = false;

    // Use ShapeCache for common shapes when scale is uniform (1,1,1)
    bool uniform_scale = std::abs(scale.x() - 1.0f) < 0.01f &&
                         std::abs(scale.y() - 1.0f) < 0.01f &&
                         std::abs(scale.z() - 1.0f) < 0.01f;

    if (entity.HasComponent<BoxCollider>()) {
        auto&     box = entity.GetComponent<BoxCollider>();
        glm::vec3 half_extents(box.Size.x * 0.5f, box.Size.y * 0.5f, box.Size.z * 0.5f);

        if (uniform_scale && box.Offset.x == 0 && box.Offset.y == 0 && box.Offset.z == 0) {
            child_shape       = ShapeCache::Instance().GetBoxShape(half_extents);
            uses_cached_shape = true;
        } else {
            child_shape = new btBoxShape(btVector3(half_extents.x, half_extents.y, half_extents.z));
        }

        offset          = btVector3(box.Offset.x, box.Offset.y, box.Offset.z);
        collision_group = box.CollisionGroup;
        collision_mask  = box.CollisionMask;
        is_trigger      = box.IsTrigger;
    } else if (entity.HasComponent<SphereCollider>()) {
        auto& sphere = entity.GetComponent<SphereCollider>();

        if (uniform_scale && sphere.Offset.x == 0 && sphere.Offset.y == 0 && sphere.Offset.z == 0) {
            child_shape       = ShapeCache::Instance().GetSphereShape(sphere.Radius);
            uses_cached_shape = true;
        } else {
            child_shape = new btSphereShape(sphere.Radius);
        }

        offset          = btVector3(sphere.Offset.x, sphere.Offset.y, sphere.Offset.z);
        collision_group = sphere.CollisionGroup;
        collision_mask  = sphere.CollisionMask;
        is_trigger      = sphere.IsTrigger;
    } else if (entity.HasComponent<CapsuleCollider>()) {
        auto& capsule = entity.GetComponent<CapsuleCollider>();

        if (uniform_scale && capsule.Offset.x == 0 && capsule.Offset.y == 0 && capsule.Offset.z ==
            0) {
            child_shape = ShapeCache::Instance().GetCapsuleShape(capsule.Radius, capsule.Height);
            uses_cached_shape = true;
        } else {
            child_shape = new btCapsuleShape(capsule.Radius, capsule.Height);
        }

        offset          = btVector3(capsule.Offset.x, capsule.Offset.y, capsule.Offset.z);
        collision_group = capsule.CollisionGroup;
        collision_mask  = capsule.CollisionMask;
        is_trigger      = capsule.IsTrigger;
    } else {
        // Default box - don't use cache since we need to apply scale
        child_shape       = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
        uses_cached_shape = false;
    }

    // Apply scale to non-cached shapes
    if (!uses_cached_shape) {
        child_shape->setLocalScaling(scale);
    }

    if (!offset.isZero()) {
        btCompoundShape* compound = new btCompoundShape();
        btTransform      local_transform;
        local_transform.setIdentity();
        btVector3 scaled_offset(offset.x() * scale.x(), offset.y() * scale.y(),
                                offset.z() * scale.z());
        local_transform.setOrigin(scaled_offset);
        compound->addChildShape(local_transform, child_shape);
        final_shape       = compound;
        uses_cached_shape = false; // Compound wrapper is not cached
    } else {
        final_shape = child_shape;
    }

    btTransform start_transform;
    start_transform.setIdentity();

    btScalar mass = 0.0f;
    if (data.type == RigidbodyType::Dynamic) {
        mass = data.mass;
    }

    btVector3 local_inertia(0.0f, 0.0f, 0.0f);
    if (mass != 0.0f) {
        final_shape->calculateLocalInertia(mass, local_inertia);
    }

    start_transform.setOrigin(btVector3(transform_component.Position.x,
                                        transform_component.Position.y,
                                        transform_component.Position.z));
    glm::quat q = glm::quat(glm::radians(transform_component.Rotation));
    start_transform.setRotation(btQuaternion(q.x, q.y, q.z, q.w));

    btDefaultMotionState* motion_state = new btDefaultMotionState(start_transform);
    btRigidBody::btRigidBodyConstructionInfo
        rb_info(mass, motion_state, final_shape, local_inertia);

    rb_info.m_friction       = data.material.Friction;
    rb_info.m_restitution    = data.material.Restitution;
    rb_info.m_linearDamping  = data.material.LinearDamping;
    rb_info.m_angularDamping = data.material.AngularDamping;

    btRigidBody* rigid_body = new btRigidBody(rb_info);

    if (data.type == RigidbodyType::Kinematic) {
        rigid_body->setCollisionFlags(
            rigid_body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        rigid_body->setActivationState(DISABLE_DEACTIVATION);
    }

    if (is_trigger) {
        rigid_body->setCollisionFlags(
            rigid_body->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
    }

    btVector3 angular_factor(
        data.freezeRotationX ? 0.0f : 1.0f,
        data.freezeRotationY ? 0.0f : 1.0f,
        data.freezeRotationZ ? 0.0f : 1.0f
        );
    rigid_body->setAngularFactor(angular_factor);
    rigid_body->setGravity(btVector3(0.0f, config_.gravity * data.gravityScale, 0.0f));

    {
        std::lock_guard<std::mutex> lock(command_queue_mutex_);
        pending_add_bodies_.push({entity, rigid_body, collision_group, collision_mask,
                                  uses_cached_shape});
    }

    return rigid_body;
}

void PhysicsSystem::RemoveRigidBody(btRigidBody* body) {
    if (!body) return;
    std::lock_guard<std::mutex> lock(command_queue_mutex_);
    pending_remove_bodies_.push(body);
}

void PhysicsSystem::RemoveBodyInternal(btRigidBody* body) {
    if (!dynamics_world_ || !body) return;

    dynamics_world_->removeRigidBody(body);

    bool uses_cached_shape = false;
    for (auto it = bodies_.begin(); it != bodies_.end(); ++it) {
        if (it->body == body) {
            uses_cached_shape = it->usesCachedShape;
            bodies_.erase(it);
            break;
        }
    }

    delete body->getMotionState();

    if (!uses_cached_shape) {
        btCollisionShape* shape = body->getCollisionShape();
        if (shape) {
            if (shape->isCompound()) {
                btCompoundShape* compound   = static_cast<btCompoundShape*>(shape);
                int              childCount = compound->getNumChildShapes();
                for (int i = childCount - 1; i >= 0; i--) {
                    // Don't delete child if it might be cached
                }
            }
            delete shape;
        }
    }

    delete body;
}

void PhysicsSystem::RenderDebug(const Camera& camera) {
    std::lock_guard<std::mutex> lock(physics_mutex_);
    if (dynamics_world_ && debug_drawer_) {
        dynamics_world_->debugDrawWorld();
        debug_drawer_->Flush(camera);
    }
}

void PhysicsSystem::UpdateDebugDraw(float dt) {
    if (debug_drawer_) {
        debug_drawer_->UpdateTimedElements(dt);
    }
}

struct RaycastCallback : public btCollisionWorld::ClosestRayResultCallback {
    btCollisionObject* m_ignoredBody;

    RaycastCallback(const btVector3&   rayFromWorld, const btVector3& rayToWorld,
                    btCollisionObject* ignoredBody)
        : btCollisionWorld::ClosestRayResultCallback(rayFromWorld, rayToWorld),
          m_ignoredBody(ignoredBody) {}

    virtual bool needsCollision(btBroadphaseProxy* proxy0) const override {
        if (proxy0->m_clientObject == m_ignoredBody) return false;
        return btCollisionWorld::ClosestRayResultCallback::needsCollision(proxy0);
    }
};

bool PhysicsSystem::Raycast(const glm::vec3& start, const glm::vec3& end, glm::vec3& hitPoint,
                            glm::vec3&       hitNormal, btRigidBody* ignoredBody) {
    std::lock_guard<std::mutex> lock(physics_mutex_);
    if (!dynamics_world_) return false;

    btVector3 btStart(start.x, start.y, start.z);
    btVector3 btEnd(end.x, end.y, end.z);

    RaycastCallback rayCallback(btStart, btEnd, ignoredBody);
    dynamics_world_->rayTest(btStart, btEnd, rayCallback);

    if (rayCallback.hasHit()) {
        hitPoint = glm::vec3(rayCallback.m_hitPointWorld.x(), rayCallback.m_hitPointWorld.y(),
                             rayCallback.m_hitPointWorld.z());
        hitNormal = glm::vec3(rayCallback.m_hitNormalWorld.x(), rayCallback.m_hitNormalWorld.y(),
                              rayCallback.m_hitNormalWorld.z());
        return true;
    }

    return false;
}

btRigidBody* PhysicsSystem::RaycastHitBody(const glm::vec3& start, const glm::vec3& end,
                                           glm::vec3&       hitPoint, btRigidBody*  ignoredBody) {
    std::lock_guard<std::mutex> lock(physics_mutex_);
    if (!dynamics_world_) return nullptr;

    btVector3 btStart(start.x, start.y, start.z);
    btVector3 btEnd(end.x, end.y, end.z);

    RaycastCallback rayCallback(btStart, btEnd, ignoredBody);
    dynamics_world_->rayTest(btStart, btEnd, rayCallback);

    if (rayCallback.hasHit()) {
        hitPoint = glm::vec3(rayCallback.m_hitPointWorld.x(), rayCallback.m_hitPointWorld.y(),
                             rayCallback.m_hitPointWorld.z());

        const btCollisionObject* hitObj = rayCallback.m_collisionObject;
        if (hitObj) {
            return const_cast<btRigidBody*>(btRigidBody::upcast(hitObj));
        }
    }

    return nullptr;
}

bool PhysicsSystem::RaycastSync(const glm::vec3& start, const glm::vec3& end, glm::vec3& hitPoint,
                                glm::vec3&       hitNormal, btRigidBody* ignoredBody) {
    return Raycast(start, end, hitPoint, hitNormal, ignoredBody);
}

btRigidBody* PhysicsSystem::RaycastHitBodySync(const glm::vec3& start, const glm::vec3& end,
                                               glm::vec3& hitPoint, btRigidBody* ignoredBody) {
    return RaycastHitBody(start, end, hitPoint, ignoredBody);
}
} // namespace se
