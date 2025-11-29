#pragma once

#include <vector>
#include <utility>

#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <btBulletCollisionCommon.h>

#include "BoxCollider.h"
#include "RigidbodyComponent.h"
#include "engine/ecs/Scene.h"

namespace se {

class PhysicsManager {
public:
    PhysicsManager();
    ~PhysicsManager();

    void Initialize();

    void Update(float delta_time);

    static BoxCollider* CreateBoxCollider(const Vector3& size);

    void SetScene(Scene& scene) {
        scene_ = &scene;
    }

private:
    friend class RigidbodyComponent;

    // Pass the pointer by reference so the caller receives the created btRigidBody*
    void AddRigidBody(btRigidBody*& rigid_body, const RigidbodyData& data, Entity entity);

private:
    btDiscreteDynamicsWorld*             dynamics_world_;
    btDefaultCollisionConfiguration*     collision_configuration_;
    btCollisionDispatcher*               dispatcher_;
    btBroadphaseInterface*               overlapping_pair_cache_broadphase_interface_;
    btSequentialImpulseConstraintSolver* solver_;
    btVector3                            gravity_      = btVector3(0.0f, -9.81f, 0.0f);
    int                                  max_substeps_ = 10;

    Scene* scene_ = nullptr;

    std::vector<std::pair<Entity, btRigidBody*>> bodies_;
};

}  // namespace se
