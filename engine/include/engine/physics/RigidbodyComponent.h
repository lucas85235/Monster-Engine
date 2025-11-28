#pragma once

#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "engine/ecs/Component.h"

namespace se {
struct RigidbodyData {
    RigidbodyData()    = default;
    float mass         = 1.0f;
    float gravityScale = 1.0f;
};

class RigidbodyComponent : public Component {
   public:
    RigidbodyComponent(const RigidbodyData& data);

    void Update(float delta_time) override;

   private:
    RigidbodyData data_;
    btRigidBody* body_ = nullptr;
};
}  // namespace se