#include "engine/physics/PhysicsSystem.h"

#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorldMt.h>
#include <BulletCollision/CollisionDispatch/btCollisionDispatcherMt.h>
#include <BulletCollision/CollisionDispatch/btDefaultCollisionConfiguration.h>
#include <LinearMath/btThreads.h>

#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/physics/BoxCollider.h"
#include "engine/physics/RigidbodyComponent.h"
#include "engine/physics/PhysicsDebugDraw.h"
#include "engine/Log.h"

#include <chrono>
#include <algorithm>

namespace se {

PhysicsSystem::PhysicsSystem(Scene* scene) : scene_(scene) {
}

PhysicsSystem::~PhysicsSystem() {
    Shutdown();
}


void PhysicsSystem::Initialize() {
    SE_LOG_INFO("Initializing PhysicsSystem (Multi-Threaded)...");

    // Init Task Scheduler
    task_scheduler_ = btCreateDefaultTaskScheduler();
    btSetTaskScheduler(task_scheduler_);

    // Bullet Init
    btDefaultCollisionConstructionInfo cci;
    // [FIX] Increased pool sizes significantly to 1,048,576 (1M) to handle very high object counts
    cci.m_defaultMaxPersistentManifoldPoolSize = 1048576;
    cci.m_defaultMaxCollisionAlgorithmPoolSize = 1048576;
    collision_configuration_ = new btDefaultCollisionConfiguration(cci);
    
    // Use MT Dispatcher
    dispatcher_ = new btCollisionDispatcherMt(collision_configuration_);
    overlapping_pair_cache_broadphase_interface_ = new btDbvtBroadphase();
    
    // Create Solver Pool
    solver_pool_ = new btConstraintSolverPoolMt(task_scheduler_->getMaxNumThreads());
    
    // Create MT World
    // Pass nullptr for the single-threaded fallback solver to avoid race conditions
    dynamics_world_ = new btDiscreteDynamicsWorldMt(
        (btDispatcher*)dispatcher_, 
        overlapping_pair_cache_broadphase_interface_, 
        solver_pool_, 
        (btConstraintSolver*)nullptr, 
        (btCollisionConfiguration*)collision_configuration_
    );

    dynamics_world_->setGravity(btVector3(0.0f, -9.81f, 0.0f));

    debug_drawer_ = new PhysicsDebugDraw();
    dynamics_world_->setDebugDrawer(debug_drawer_);

    // Start Thread
    running_ = true;
    physics_thread_ = std::thread(&PhysicsSystem::PhysicsLoop, this);
}

// -----------------------------------------------------------------------------------------------------------------------------
// [MODIFIED] Shutdown: Clean up solver and recursively delete shapes.
// -----------------------------------------------------------------------------------------------------------------------------
void PhysicsSystem::Shutdown() {
    if (running_) {
        running_ = false;
        if (physics_thread_.joinable()) {
            physics_thread_.join();
        }
    }

    // Cleanup Bullet
    if (dynamics_world_) {
        // Remove bodies
        for (auto& entry : bodies_) {
            if (entry.body) {
                dynamics_world_->removeRigidBody(entry.body);
                delete entry.body->getMotionState();
                
                // [FIX] Recursively delete shapes to avoid memory leaks
                btCollisionShape* shape = entry.body->getCollisionShape();
                if (shape) {
                    if (shape->isCompound()) {
                        btCompoundShape* compound = static_cast<btCompoundShape*>(shape);
                        int childCount = compound->getNumChildShapes();
                        for (int i = childCount - 1; i >= 0; i--) {
                            btCollisionShape* child = compound->getChildShape(i);
                            delete child;
                        }
                    }
                    delete shape;
                }
                
                delete entry.body;
            }
        }
        bodies_.clear();

        delete dynamics_world_;
        // solver_ is nullptr in MT setup, delete the pool instead
        delete solver_pool_;
        delete overlapping_pair_cache_broadphase_interface_;
        delete dispatcher_;
        delete collision_configuration_;
        delete debug_drawer_;
        
        // Cleanup Scheduler
        btSetTaskScheduler(nullptr);
        delete task_scheduler_;

        dynamics_world_ = nullptr;
    }
}

void PhysicsSystem::PhysicsLoop() {
    SE_LOG_INFO("Physics Thread Started");

    using Clock = std::chrono::high_resolution_clock;
    auto last_time = Clock::now();
    const float fixed_step = 1.0f / 60.0f;

    while (running_) {
        auto current_time = Clock::now();
        std::chrono::duration<float> delta = current_time - last_time;
        last_time = current_time;

        float dt = delta.count();
        if (dt > 0.1f) dt = 0.1f;

        {
            std::lock_guard<std::mutex> lock(physics_mutex_);
            if (dynamics_world_) {
                // Process queued add/remove operations before simulation step
                ProcessPendingCommands();
                
                auto start_physics = Clock::now();
                dynamics_world_->stepSimulation(dt, 4, fixed_step);
                auto end_physics = Clock::now();
                std::chrono::duration<float, std::milli> physics_duration = end_physics - start_physics;
                last_physics_execution_time_ = physics_duration.count();
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    SE_LOG_INFO("Physics Thread Stopped");
}

void PhysicsSystem::ProcessPendingCommands() {
    std::lock_guard<std::mutex> lock(command_queue_mutex_);
    
    // Process removals first
    while (!pending_remove_bodies_.empty()) {
        btRigidBody* body = pending_remove_bodies_.front();
        pending_remove_bodies_.pop();
        RemoveBodyInternal(body);
    }
    
    // Process additions
    while (!pending_add_bodies_.empty()) {
        auto& pending = pending_add_bodies_.front();
        
        // Actually add the body to the dynamics world now (safe, on physics thread)
        dynamics_world_->addRigidBody(pending.body, pending.collision_group, pending.collision_mask);
        bodies_.push_back({pending.entity, pending.body});
        
        pending_add_bodies_.pop();
    }
}

// -----------------------------------------------------------------------------------------------------------------------------
// Update(float dt) remains unchanged ...
// -----------------------------------------------------------------------------------------------------------------------------
void PhysicsSystem::Update(float dt) {
    // Sync Transforms from Physics to ECS
    std::lock_guard<std::mutex> lock(physics_mutex_);

    for (auto& entry : bodies_) {
        if (!entry.body) continue;
        
        // Check if entity still valid/alive? 
        // For now assume yes.

        auto& transform_component = entry.entity.GetComponent<TransformComponent>();
        
        btTransform trans;

        if (entry.body) {
            trans = entry.body->getInterpolationWorldTransform();
        }

        const btVector3& origin = trans.getOrigin();
        transform_component.Position.x = static_cast<float>(origin.getX());
        transform_component.Position.y = static_cast<float>(origin.getY());
        transform_component.Position.z = static_cast<float>(origin.getZ());

        const btQuaternion& rot = trans.getRotation();
        glm::quat q(static_cast<float>(rot.w()), static_cast<float>(rot.x()), static_cast<float>(rot.y()), static_cast<float>(rot.z()));
        q = glm::normalize(q);

        float pitch = glm::pitch(q);
        float yaw   = glm::yaw(q);
        float roll  = glm::roll(q);

        transform_component.Rotation.x = glm::degrees(pitch);
        transform_component.Rotation.y = glm::degrees(yaw);
        transform_component.Rotation.z = glm::degrees(roll);
        
        // Invalidate cached transform matrix after physics update
        transform_component.MarkDirty();
    }
}


// ... AddRigidBody remains mostly same, returning to RemoveRigidBody ...

btRigidBody* PhysicsSystem::AddRigidBody(Entity entity, const RigidbodyData& data) {
    std::lock_guard<std::mutex> lock(physics_mutex_);

    if (!dynamics_world_) return nullptr;

    auto& transform_component = entity.GetComponent<TransformComponent>();
    btVector3 scale(transform_component.Scale.x, transform_component.Scale.y, transform_component.Scale.z);

    btCollisionShape* final_shape = nullptr;
    btCollisionShape* child_shape = nullptr;
    btVector3 offset(0, 0, 0);
    uint16_t collision_group = 0x0001;
    uint16_t collision_mask = 0xFFFF;
    bool is_trigger = false;

    if (entity.HasComponent<BoxCollider>()) {
        auto& box = entity.GetComponent<BoxCollider>();
        child_shape = new btBoxShape(btVector3(box.Size.x * 0.5f, box.Size.y * 0.5f, box.Size.z * 0.5f));
        offset = btVector3(box.Offset.x, box.Offset.y, box.Offset.z);
        collision_group = box.CollisionGroup;
        collision_mask = box.CollisionMask;
        is_trigger = box.IsTrigger;
    } else if (entity.HasComponent<SphereCollider>()) {
        auto& sphere = entity.GetComponent<SphereCollider>();
        child_shape = new btSphereShape(sphere.Radius);
        offset = btVector3(sphere.Offset.x, sphere.Offset.y, sphere.Offset.z);
        collision_group = sphere.CollisionGroup;
        collision_mask = sphere.CollisionMask;
        is_trigger = sphere.IsTrigger;
    } else if (entity.HasComponent<CapsuleCollider>()) {
        auto& capsule = entity.GetComponent<CapsuleCollider>();
        child_shape = new btCapsuleShape(capsule.Radius, capsule.Height);
        offset = btVector3(capsule.Offset.x, capsule.Offset.y, capsule.Offset.z);
        collision_group = capsule.CollisionGroup;
        collision_mask = capsule.CollisionMask;
        is_trigger = capsule.IsTrigger;
    } else {
        child_shape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
    }

    child_shape->setLocalScaling(scale);

    if (!offset.isZero()) {
        btCompoundShape* compound = new btCompoundShape();
        btTransform local_transform;
        local_transform.setIdentity();
        btVector3 scaled_offset(offset.x() * scale.x(), offset.y() * scale.y(), offset.z() * scale.z());
        local_transform.setOrigin(scaled_offset); 
        compound->addChildShape(local_transform, child_shape);
        final_shape = compound;
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

    start_transform.setOrigin(btVector3(transform_component.Position.x, transform_component.Position.y, transform_component.Position.z));
    glm::quat q = glm::quat(glm::radians(transform_component.Rotation));
    start_transform.setRotation(btQuaternion(q.x, q.y, q.z, q.w));

    btDefaultMotionState* motion_state = new btDefaultMotionState(start_transform);
    btRigidBody::btRigidBodyConstructionInfo rb_info(mass, motion_state, final_shape, local_inertia);

    rb_info.m_friction = data.material.Friction;
    rb_info.m_restitution = data.material.Restitution;
    rb_info.m_linearDamping = data.material.LinearDamping;
    rb_info.m_angularDamping = data.material.AngularDamping;

    btRigidBody* rigid_body = new btRigidBody(rb_info);

    if (data.type == RigidbodyType::Kinematic) {
        rigid_body->setCollisionFlags(rigid_body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        rigid_body->setActivationState(DISABLE_DEACTIVATION);
    }

    if (is_trigger) {
        rigid_body->setCollisionFlags(rigid_body->getCollisionFlags() | btCollisionObject::CF_NO_CONTACT_RESPONSE);
    }

    btVector3 angular_factor(
        data.freezeRotationX ? 0.0f : 1.0f,
        data.freezeRotationY ? 0.0f : 1.0f,
        data.freezeRotationZ ? 0.0f : 1.0f
    );
    rigid_body->setAngularFactor(angular_factor);

    rigid_body->setGravity(btVector3(0.0f, -9.81f * data.gravityScale, 0.0f));

    // Queue the body for addition on the physics thread
    {
        std::lock_guard<std::mutex> lock(command_queue_mutex_);
        pending_add_bodies_.push({entity, rigid_body, collision_group, collision_mask});
    }

    SE_LOG_INFO("Queued rigidbody for add: mass={}, type={}, friction={}", 
                mass, static_cast<int>(data.type), data.material.Friction);

    return rigid_body;
}

// -----------------------------------------------------------------------------------------------------------------------------
// [MODIFIED] RemoveRigidBody: Cleanup shapes recursively.
// -----------------------------------------------------------------------------------------------------------------------------
void PhysicsSystem::RemoveRigidBody(btRigidBody* body) {
    if (!body) return;
    
    std::lock_guard<std::mutex> lock(command_queue_mutex_);
    pending_remove_bodies_.push(body);
    SE_LOG_INFO("Queued rigidbody for removal");
}

void PhysicsSystem::RemoveBodyInternal(btRigidBody* body) {
    // Called from physics thread inside ProcessPendingCommands with physics_mutex_ already held
    if (!dynamics_world_ || !body) return;

    dynamics_world_->removeRigidBody(body);
    
    // Remove from bodies_ list
    for (auto it = bodies_.begin(); it != bodies_.end(); ++it) {
        if (it->body == body) {
            bodies_.erase(it);
            break;
        }
    }

    delete body->getMotionState();
    
    btCollisionShape* shape = body->getCollisionShape();
    if (shape) {
        if (shape->isCompound()) {
            btCompoundShape* compound = static_cast<btCompoundShape*>(shape);
            int childCount = compound->getNumChildShapes();
            for (int i = childCount - 1; i >= 0; i--) {
                btCollisionShape* child = compound->getChildShape(i);
                delete child;
            }
        }
        delete shape;
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

        RaycastCallback(const btVector3& rayFromWorld, const btVector3& rayToWorld, btCollisionObject* ignoredBody)
            : btCollisionWorld::ClosestRayResultCallback(rayFromWorld, rayToWorld), m_ignoredBody(ignoredBody) {}

        virtual bool needsCollision(btBroadphaseProxy* proxy0) const override {
            if (proxy0->m_clientObject == m_ignoredBody) return false;
            return btCollisionWorld::ClosestRayResultCallback::needsCollision(proxy0);
        }
    };

    bool PhysicsSystem::Raycast(const glm::vec3& start, const glm::vec3& end, glm::vec3& hitPoint, glm::vec3& hitNormal, btRigidBody* ignoredBody) {
        std::lock_guard<std::mutex> lock(physics_mutex_);
        if (!dynamics_world_) return false;

        btVector3 btStart(start.x, start.y, start.z);
        btVector3 btEnd(end.x, end.y, end.z);

        RaycastCallback rayCallback(btStart, btEnd, ignoredBody);
        dynamics_world_->rayTest(btStart, btEnd, rayCallback);

        if (rayCallback.hasHit()) {
            hitPoint = glm::vec3(rayCallback.m_hitPointWorld.x(), rayCallback.m_hitPointWorld.y(), rayCallback.m_hitPointWorld.z());
            hitNormal = glm::vec3(rayCallback.m_hitNormalWorld.x(), rayCallback.m_hitNormalWorld.y(), rayCallback.m_hitNormalWorld.z());
            return true;
        }

        return false;
    }

    btRigidBody* PhysicsSystem::RaycastHitBody(const glm::vec3& start, const glm::vec3& end, glm::vec3& hitPoint, btRigidBody* ignoredBody) {
        std::lock_guard<std::mutex> lock(physics_mutex_);
        if (!dynamics_world_) return nullptr;

        btVector3 btStart(start.x, start.y, start.z);
        btVector3 btEnd(end.x, end.y, end.z);

        RaycastCallback rayCallback(btStart, btEnd, ignoredBody);
        dynamics_world_->rayTest(btStart, btEnd, rayCallback);

        if (rayCallback.hasHit()) {
            hitPoint = glm::vec3(rayCallback.m_hitPointWorld.x(), rayCallback.m_hitPointWorld.y(), rayCallback.m_hitPointWorld.z());
            
            const btCollisionObject* hitObj = rayCallback.m_collisionObject;
            if (hitObj) {
                return const_cast<btRigidBody*>(btRigidBody::upcast(hitObj));
            }
        }

        return nullptr;
    }

} // namespace se
