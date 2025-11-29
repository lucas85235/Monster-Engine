#include "engine/physics/BoxCollider.h"

#include "Engine.h"
namespace se {
BoxCollider::BoxCollider(const Vector3& size) {
    collider_ = new btBoxShape(btVector3(size.x, size.y, size.z));
}

void BoxCollider::Update(float delta_time) {}

}  // namespace se