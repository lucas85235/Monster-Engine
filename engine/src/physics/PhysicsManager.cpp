#include "engine/physics/PhysicsManager.h"

#include <cstdio>

#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/physics/BoxCollider.h"
#include "engine/physics/RigidbodyComponent.h"

namespace se {

PhysicsManager::PhysicsManager()
    : dynamics_world_(nullptr),
      collision_configuration_(nullptr),
      dispatcher_(nullptr),
      overlapping_pair_cache_broadphase_interface_(nullptr),
      solver_(nullptr),
      scene_(nullptr) {}

PhysicsManager::~PhysicsManager() {
    if (dynamics_world_) {
        // Remove and destroy all rigid bodies
        for (auto& pair : bodies_) {
            btRigidBody* body = pair.second;
            if (!body) { continue; }

            dynamics_world_->removeRigidBody(body);

            btMotionState* motion_state = body->getMotionState();
            if (motion_state) { delete motion_state; }

            btCollisionShape* shape = body->getCollisionShape();
            if (shape) { delete shape; }

            delete body;
        }

        bodies_.clear();
    }

    delete dynamics_world_;
    delete solver_;
    delete overlapping_pair_cache_broadphase_interface_;
    delete dispatcher_;
    delete collision_configuration_;

    dynamics_world_                              = nullptr;
    solver_                                      = nullptr;
    overlapping_pair_cache_broadphase_interface_ = nullptr;
    dispatcher_                                  = nullptr;
    collision_configuration_                     = nullptr;
}

void PhysicsManager::Initialize() {
    // Default collision configuration for memory and collision setup
    collision_configuration_ = new btDefaultCollisionConfiguration();

    // Default collision dispatcher
    dispatcher_ = new btCollisionDispatcher(collision_configuration_);

    // General-purpose broadphase
    overlapping_pair_cache_broadphase_interface_ = new btDbvtBroadphase();

    // Default constraint solver
    solver_ = new btSequentialImpulseConstraintSolver;

    // Create dynamics world
    dynamics_world_ = new btDiscreteDynamicsWorld(dispatcher_, overlapping_pair_cache_broadphase_interface_, solver_, collision_configuration_);

    dynamics_world_->setGravity(gravity_);
}

void PhysicsManager::Update(float delta_time) {
    if (!dynamics_world_) { return; }

    dynamics_world_->stepSimulation(delta_time, max_substeps_);

    // Sync TransformComponent with Bullet rigid bodies
    for (auto& pair : bodies_) {
        Entity       entity = pair.first;
        btRigidBody* body   = pair.second;

        if (!body) { continue; }

        auto& transform_component = entity.GetComponent<TransformComponent>();

        btTransform trans;
        if (body->getMotionState()) {
            body->getMotionState()->getWorldTransform(trans);
        } else {
            trans = body->getWorldTransform();
        }

        // --- Position ---
        const btVector3& origin        = trans.getOrigin();
        transform_component.Position.x = static_cast<float>(origin.getX());
        transform_component.Position.y = static_cast<float>(origin.getY());
        transform_component.Position.z = static_cast<float>(origin.getZ());

        // --- Rotation (Bullet quaternion -> Euler degrees) ---
        const btQuaternion& rot = trans.getRotation();

        glm::quat q(static_cast<float>(rot.w()), static_cast<float>(rot.x()), static_cast<float>(rot.y()), static_cast<float>(rot.z()));

        // GLM returns radians here
        glm::vec3 euler_rad = glm::eulerAngles(q);
        glm::vec3 euler_deg = glm::degrees(euler_rad);

        transform_component.Rotation.x = euler_deg.x;
        transform_component.Rotation.y = euler_deg.y;
        transform_component.Rotation.z = euler_deg.z;

        // --- Scale (from collision shape local scaling) ---
        btCollisionShape* shape = body->getCollisionShape();
        if (shape) {
            const btVector3& scaling    = shape->getLocalScaling();
            transform_component.Scale.x = static_cast<float>(scaling.getX());
            transform_component.Scale.y = static_cast<float>(scaling.getY());
            transform_component.Scale.z = static_cast<float>(scaling.getZ());
        }

        // Optional debug
        std::printf("pos = %f,%f,%f | rot = %f,%f,%f | scale = %f,%f,%f\n",
                    transform_component.Position.x,
                    transform_component.Position.y,
                    transform_component.Position.z,
                    transform_component.Rotation.x,
                    transform_component.Rotation.y,
                    transform_component.Rotation.z,
                    transform_component.Scale.x,
                    transform_component.Scale.y,
                    transform_component.Scale.z);
    }
}

BoxCollider* PhysicsManager::CreateBoxCollider(const Vector3& size) {
    return new BoxCollider(size);
}

void PhysicsManager::AddRigidBody(btRigidBody*& rigid_body, const RigidbodyData& data, Entity entity) {
    if (!dynamics_world_) { return; }

    auto& transform_component = entity.GetComponent<TransformComponent>();

    // TODO: integrate with the entity's BoxCollider component and use the correct size
    btCollisionShape* internal_collider = new btBoxShape(btVector3(1.0f, 1.0f, 1.0f));

    btTransform start_transform;
    start_transform.setIdentity();

    btScalar mass = data.mass;

    bool is_dynamic = (mass != 0.0f);

    btVector3 local_inertia(0.0f, 0.0f, 0.0f);
    if (is_dynamic) { internal_collider->calculateLocalInertia(mass, local_inertia); }

    start_transform.setOrigin(btVector3(transform_component.Position.x, transform_component.Position.y, transform_component.Position.z));

    // Using a motion state is recommended for interpolation and syncing only active objects
    btDefaultMotionState*                    motion_state = new btDefaultMotionState(start_transform);
    btRigidBody::btRigidBodyConstructionInfo rb_info(mass, motion_state, internal_collider, local_inertia);

    rigid_body = new btRigidBody(rb_info);

    // Store the (Entity, RigidBody) pair
    bodies_.emplace_back(entity, rigid_body);

    dynamics_world_->addRigidBody(rigid_body);
}

}  // namespace se
