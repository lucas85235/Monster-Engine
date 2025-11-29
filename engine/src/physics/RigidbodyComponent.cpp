#include "engine/physics/RigidbodyComponent.h"

#include "engine/Application.h"

namespace se {
RigidbodyComponent::RigidbodyComponent(const RigidbodyData& data, Entity entity) : data_(data) {
    Application::Get().GetPhysicsManager().AddRigidBody(body_, data, entity);
}

void RigidbodyComponent::Update(float delta_time) {}
void RigidbodyComponent::AddForce(const btVector3& force, const btVector3& point) {
    body_->applyForce(force, point);
}
void RigidbodyComponent::AddImpulse(const btVector3& impulse, const btVector3& point) {
    body_->applyImpulse(impulse,point);
}

}  // namespace se