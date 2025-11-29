#pragma once

#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "engine/ecs/Component.h"

namespace se {
struct RigidbodyData {
    float mass         = 1.0f;
    float gravityScale = 1.0f;
};

class RigidbodyComponent : public Component {
   public:
    RigidbodyComponent(const RigidbodyData& data, Entity entity);

    void Update(float delta_time) override;

    btRigidBody* GetRigidbody() const {
        return body_;
    }

    void AddForce(const btVector3& force, const btVector3& point);
    void AddImpulse(const btVector3& impulse, const btVector3& point);
   private:
    RigidbodyData data_;
    btRigidBody* body_ = nullptr;
};
}  // namespace se