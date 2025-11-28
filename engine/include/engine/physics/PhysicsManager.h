#pragma once
#include <BulletDynamics/ConstraintSolver/btSequentialImpulseConstraintSolver.h>
#include <BulletDynamics/Dynamics/btDiscreteDynamicsWorld.h>
#include <btBulletCollisionCommon.h>

namespace se {
class PhysicsManager {
   public:
    PhysicsManager() = default;
    inline void Initialize() {
        int i;
        ///-----initialization_start-----

        /// collision configuration contains default setup for memory, collision setup. Advanced users can create their own configuration.
        collision_configuration_ = new btDefaultCollisionConfiguration();

        /// use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
        dispatcher_ = new btCollisionDispatcher(collision_configuration_);

        /// btDbvtBroadphase is a good general purpose broadphase. You can also try out btAxis3Sweep.
        overlapping_pair_cache_broadphase_interface_ = new btDbvtBroadphase();

        /// the default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
        solver_ = new btSequentialImpulseConstraintSolver;

        dynamics_world_ = new btDiscreteDynamicsWorld(dispatcher_, overlapping_pair_cache_broadphase_interface_, solver_, collision_configuration_);

        dynamics_world_->setGravity(gravity_);

        // btAlignedObjectArray<btCollisionShape*> collisionShapes;
        //
        // {
        //     btCollisionShape* groundShape = new btBoxShape(btVector3(btScalar(50.), btScalar(50.), btScalar(50.)));
        //
        //     collisionShapes.push_back(groundShape);
        //
        //     btTransform groundTransform;
        //     groundTransform.setIdentity();
        //     groundTransform.setOrigin(btVector3(0, -56, 0));
        //
        //     btScalar mass(0.);
        //
        //     // rigidbody is dynamic if and only if mass is non zero, otherwise static
        //     bool isDynamic = (mass != 0.f);
        //
        //     btVector3 localInertia(0, 0, 0);
        //     if (isDynamic) groundShape->calculateLocalInertia(mass, localInertia);
        //
        //     // using motionstate is optional, it provides interpolation capabilities, and only synchronizes 'active' objects
        //     btDefaultMotionState*                    myMotionState = new btDefaultMotionState(groundTransform);
        //     btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, groundShape, localInertia);
        //     btRigidBody*                             body = new btRigidBody(rbInfo);
        //
        //     // add the body to the dynamics world
        //     dynamics_world_->addRigidBody(body);
        // }
        //
        // {
        //     // create a dynamic rigidbody
        //
        //     // btCollisionShape* colShape = new btBoxShape(btVector3(1,1,1));
        //     btCollisionShape* colShape = new btSphereShape(btScalar(1.));
        //     collisionShapes.push_back(colShape);
        //
        //     /// Create Dynamic Objects
        //     btTransform startTransform;
        //     startTransform.setIdentity();
        //
        //     btScalar mass(1.f);
        //
        //     // rigidbody is dynamic if and only if mass is non zero, otherwise static
        //     bool isDynamic = (mass != 0.f);
        //
        //     btVector3 localInertia(0, 0, 0);
        //     if (isDynamic) colShape->calculateLocalInertia(mass, localInertia);
        //
        //     startTransform.setOrigin(btVector3(2, 10, 0));
        //
        //     // using motionstate is recommended, it provides interpolation capabilities, and only synchronizes 'active' objects
        //     btDefaultMotionState*                    myMotionState = new btDefaultMotionState(startTransform);
        //     btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, myMotionState, colShape, localInertia);
        //     btRigidBody*                             body = new btRigidBody(rbInfo);
        //
        //     dynamics_world_->addRigidBody(body);
        // }
        //
        // /// Do some simulation
        //
        // ///-----stepsimulation_start-----
        // for (i = 0; i < 150; i++) {
        //     dynamics_world_->stepSimulation(1.f / 60.f, 10);
        //
        //     // print positions of all objects
        //     for (int j = dynamics_world_->getNumCollisionObjects() - 1; j >= 0; j--) {
        //         btCollisionObject* obj  = dynamics_world_->getCollisionObjectArray()[j];
        //         btRigidBody*       body = btRigidBody::upcast(obj);
        //         btTransform        trans;
        //         if (body && body->getMotionState()) {
        //             body->getMotionState()->getWorldTransform(trans);
        //         } else {
        //             trans = obj->getWorldTransform();
        //         }
        //         printf("world pos object %d = %f,%f,%f\n", j, float(trans.getOrigin().getX()), float(trans.getOrigin().getY()),
        //                float(trans.getOrigin().getZ()));
        //     }
        // }
    }
    void Update(float delta_time);

   private:
    friend class RigidbodyComponent;
    void AddRigidBody(btRigidBody& rigid_body);

   private:
    btDiscreteDynamicsWorld*             dynamics_world_;
    btDefaultCollisionConfiguration*     collision_configuration_;
    btCollisionDispatcher*               dispatcher_;
    btBroadphaseInterface*               overlapping_pair_cache_broadphase_interface_;
    btSequentialImpulseConstraintSolver* solver_;
    btVector3                            gravity_      = btVector3(0, -9.81f, 0);
    int                                  max_substeps_ = 10;
};
}  // namespace se