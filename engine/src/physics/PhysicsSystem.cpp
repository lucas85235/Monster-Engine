#include "engine/physics/PhysicsSystem.h"

#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/physics/BoxCollider.h"
#include "engine/physics/RigidbodyComponent.h"
#include "engine/physics/PhysicsDebugDraw.h"
#include "engine/Log.h"

#include <chrono>

namespace se {

PhysicsSystem::PhysicsSystem(Scene* scene) : scene_(scene) {
}

PhysicsSystem::~PhysicsSystem() {
    Shutdown();
}

void PhysicsSystem::Initialize() {
    SE_LOG_INFO("Initializing PhysicsSystem...");

    // Bullet Init
    collision_configuration_ = new btDefaultCollisionConfiguration();
    dispatcher_ = new btCollisionDispatcher(collision_configuration_);
    overlapping_pair_cache_broadphase_interface_ = new btDbvtBroadphase();
    solver_ = new btSequentialImpulseConstraintSolver;
    dynamics_world_ = new btDiscreteDynamicsWorld(dispatcher_, overlapping_pair_cache_broadphase_interface_, solver_, collision_configuration_);

    dynamics_world_->setGravity(btVector3(0.0f, -9.81f, 0.0f));

    debug_drawer_ = new PhysicsDebugDraw();
    dynamics_world_->setDebugDrawer(debug_drawer_);

    // Start Thread
    running_ = true;
    physics_thread_ = std::thread(&PhysicsSystem::PhysicsLoop, this);
}

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
                delete entry.body->getCollisionShape(); // Note: Compound shapes might need recursive delete if not managed
                delete entry.body;
            }
        }
        bodies_.clear();

        delete dynamics_world_;
        delete solver_;
        delete overlapping_pair_cache_broadphase_interface_;
        delete dispatcher_;
        delete collision_configuration_;
        delete debug_drawer_;

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

        // Simple fixed step loop
        // In a real engine, we might want to accumulate time or sleep to save CPU
        // For now, we just step if enough time passed, or sleep briefly
        
        float dt = delta.count();
        
        // Clamp dt to avoid spiral of death
        if (dt > 0.1f) dt = 0.1f;

        {
            std::lock_guard<std::mutex> lock(physics_mutex_);
            if (dynamics_world_) {
                auto start_physics = Clock::now();
                dynamics_world_->stepSimulation(dt, 10, fixed_step);
                auto end_physics = Clock::now();
                std::chrono::duration<float, std::milli> physics_duration = end_physics - start_physics;
                last_physics_execution_time_ = physics_duration.count();
            }
        }

        // Sleep a bit to target ~60Hz if we are running too fast
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
    
    SE_LOG_INFO("Physics Thread Stopped");
}

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
    }
}

btRigidBody* PhysicsSystem::AddRigidBody(Entity entity, const RigidbodyData& data) {
    std::lock_guard<std::mutex> lock(physics_mutex_);

    if (!dynamics_world_) return nullptr;

    auto& transform_component = entity.GetComponent<TransformComponent>();
    btVector3 scale(transform_component.Scale.x, transform_component.Scale.y, transform_component.Scale.z);

    btCollisionShape* final_shape = nullptr;
    btCollisionShape* child_shape = nullptr;
    btVector3         offset(0, 0, 0);

    // Reuse logic from PhysicsManager (simplified)
    if (entity.HasComponent<BoxCollider>()) {
        auto& box = entity.GetComponent<BoxCollider>();
        child_shape = new btBoxShape(btVector3(box.Size.x * 0.5f, box.Size.y * 0.5f, box.Size.z * 0.5f));
        offset = btVector3(box.Offset.x, box.Offset.y, box.Offset.z);
    } else if (entity.HasComponent<SphereCollider>()) {
        auto& sphere = entity.GetComponent<SphereCollider>();
        child_shape = new btSphereShape(sphere.Radius);
        offset = btVector3(sphere.Offset.x, sphere.Offset.y, sphere.Offset.z);
    } else if (entity.HasComponent<CapsuleCollider>()) {
        auto& capsule = entity.GetComponent<CapsuleCollider>();
        child_shape = new btCapsuleShape(capsule.Radius, capsule.Height);
        offset = btVector3(capsule.Offset.x, capsule.Offset.y, capsule.Offset.z);
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

    btScalar mass = data.mass;
    bool is_dynamic = (mass != 0.0f);

    btVector3 local_inertia(0.0f, 0.0f, 0.0f);
    if (is_dynamic) { final_shape->calculateLocalInertia(mass, local_inertia); }

    start_transform.setOrigin(btVector3(transform_component.Position.x, transform_component.Position.y, transform_component.Position.z));
    glm::quat q = glm::quat(glm::radians(transform_component.Rotation));
    start_transform.setRotation(btQuaternion(q.x, q.y, q.z, q.w));

    btDefaultMotionState* motion_state = new btDefaultMotionState(start_transform);
    btRigidBody::btRigidBodyConstructionInfo rb_info(mass, motion_state, final_shape, local_inertia);

    btRigidBody* rigid_body = new btRigidBody(rb_info);
    
    dynamics_world_->addRigidBody(rigid_body);
    
    bodies_.push_back({entity, rigid_body});

    return rigid_body;
}

void PhysicsSystem::RemoveRigidBody(btRigidBody* body) {
    std::lock_guard<std::mutex> lock(physics_mutex_);
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
    delete body->getCollisionShape();
    delete body;
}

    void PhysicsSystem::RenderDebug(const Camera& camera) {
        std::lock_guard<std::mutex> lock(physics_mutex_);
        if (dynamics_world_ && debug_drawer_) {
            dynamics_world_->debugDrawWorld();
            debug_drawer_->Flush(camera);
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

} // namespace se
