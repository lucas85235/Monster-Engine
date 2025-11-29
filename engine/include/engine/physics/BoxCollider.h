#pragma once
#include <btBulletCollisionCommon.h>

#include "engine/ecs/Component.h"

namespace se {
class BoxCollider : Component {
   public:
    BoxCollider(const Vector3& size);

    btCollisionShape* GetCollisionShape() const {
        return collider_;
    }

    void Update(float delta_time) override;

   private:
    btCollisionShape* collider_ = nullptr;
};
}  // namespace se