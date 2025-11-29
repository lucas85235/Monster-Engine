#include "engine/physics/PhysicsManager.h"

#include <cstdio>

#include "engine/ecs/Scene.h"
#include "engine/ecs/SimpleComponents.h"
#include "engine/physics/BoxCollider.h"
#include "engine/physics/RigidbodyComponent.h"
#include "engine/physics/PhysicsDebugDraw.h"

namespace se {

PhysicsManager::PhysicsManager()
    : dynamics_world_(nullptr),
      collision_configuration_(nullptr),
      dispatcher_(nullptr),
      overlapping_pair_cache_broadphase_interface_(nullptr),
      solver_(nullptr),
      scene_(nullptr),
      debug_drawer_(nullptr) {}

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
            if (shape) {
                if (shape->isCompound()) {
                    btCompoundShape* compound = static_cast<btCompoundShape*>(shape);
                    int numChildShapes = compound->getNumChildShapes();
                    for (int i = 0; i < numChildShapes; i++) {
                        btCollisionShape* child = compound->getChildShape(i);
                        delete child;
                    }
                }
                delete shape;
            }

            delete body;
        }

        bodies_.clear();
    }

    delete dynamics_world_;
    delete solver_;
    delete overlapping_pair_cache_broadphase_interface_;
    delete dispatcher_;
    delete collision_configuration_;
    delete debug_drawer_;

    dynamics_world_                              = nullptr;
    solver_                                      = nullptr;
    overlapping_pair_cache_broadphase_interface_ = nullptr;
    dispatcher_                                  = nullptr;
    collision_configuration_                     = nullptr;
    debug_drawer_                                = nullptr;
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

    // Initialize debug drawer
    debug_drawer_ = new PhysicsDebugDraw();
    dynamics_world_->setDebugDrawer(debug_drawer_);
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
        q = glm::normalize(q);

        // GLM returns radians here
        // We use explicit getters for better stability than eulerAngles which can be ambiguous
        float pitch = glm::pitch(q);
        float yaw   = glm::yaw(q);
        float roll  = glm::roll(q);

        transform_component.Rotation.x = glm::degrees(pitch);
        transform_component.Rotation.y = glm::degrees(yaw);
        transform_component.Rotation.z = glm::degrees(roll);

        // --- Scale (from collision shape local scaling) ---
        // Note: We generally don't want physics to drive scale unless we have specific logic for it
        // But if we do, we should be careful. 
        // For now, let's keep it as is or remove it if it causes issues. 
        // The original code had it, so I'll leave it but it might be redundant if we don't change scale in physics.
        btCollisionShape* shape = body->getCollisionShape();
        if (shape) {
            const btVector3& scaling    = shape->getLocalScaling();
            // transform_component.Scale.x = static_cast<float>(scaling.getX());
            // transform_component.Scale.y = static_cast<float>(scaling.getY());
            // transform_component.Scale.z = static_cast<float>(scaling.getZ());
        }
    }
}

void PhysicsManager::RenderDebug(const Camera& camera) {
    if (dynamics_world_ && debug_drawer_) {
        dynamics_world_->debugDrawWorld();
        debug_drawer_->Flush(camera);
    }
}

BoxCollider* PhysicsManager::CreateBoxCollider(const Vector3& size) {
    return new BoxCollider(size);
}

void PhysicsManager::AddRigidBody(btRigidBody*& rigid_body, const RigidbodyData& data, Entity entity) {
    if (!dynamics_world_) { return; }

    auto& transform_component = entity.GetComponent<TransformComponent>();
    btVector3 scale(transform_component.Scale.x, transform_component.Scale.y, transform_component.Scale.z);

    btCollisionShape* final_shape = nullptr;
    btCollisionShape* child_shape = nullptr;
    btVector3         offset(0, 0, 0);

    if (entity.HasComponent<BoxCollider>()) {
        auto& box = entity.GetComponent<BoxCollider>();
        // Box half-extents
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
        // Default fallback
        child_shape = new btBoxShape(btVector3(0.5f, 0.5f, 0.5f));
    }

    // Apply scale to the child shape
    child_shape->setLocalScaling(scale);

    // If there is an offset, we need a compound shape
    if (!offset.isZero()) {
        btCompoundShape* compound = new btCompoundShape();
        btTransform local_transform;
        local_transform.setIdentity();
        
        // Element-wise multiplication for offset scaling
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
    
    // Set initial rotation
    glm::quat q = glm::quat(glm::radians(transform_component.Rotation));
    start_transform.setRotation(btQuaternion(q.x, q.y, q.z, q.w));

    // Using a motion state is recommended for interpolation and syncing only active objects
    btDefaultMotionState*                    motion_state = new btDefaultMotionState(start_transform);
    btRigidBody::btRigidBodyConstructionInfo rb_info(mass, motion_state, final_shape, local_inertia);

    rigid_body = new btRigidBody(rb_info);

    // Store the (Entity, RigidBody) pair
    bodies_.emplace_back(entity, rigid_body);

    dynamics_world_->addRigidBody(rigid_body);
}

}  // namespace se
