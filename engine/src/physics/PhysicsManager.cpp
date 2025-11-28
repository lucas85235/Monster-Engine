#include "engine/physics/PhysicsManager.h"

namespace se {
void PhysicsManager::Update(float delta_time) {
    dynamics_world_->stepSimulation(delta_time, max_substeps_);

    for (int j = dynamics_world_->getNumCollisionObjects() - 1; j >= 0; j--) {
        btCollisionObject* obj  = dynamics_world_->getCollisionObjectArray()[j];
        btRigidBody*       body = btRigidBody::upcast(obj);
        btTransform        trans;
        if (body && body->getMotionState()) {
            body->getMotionState()->getWorldTransform(trans);
        } else {
            trans = obj->getWorldTransform();
        }
        printf("world pos object %d = %f,%f,%f\n", j, float(trans.getOrigin().getX()), float(trans.getOrigin().getY()),
               float(trans.getOrigin().getZ()));
    }
}
void PhysicsManager::AddRigidBody(btRigidBody& rigid_body) {

}
}  // namespace se