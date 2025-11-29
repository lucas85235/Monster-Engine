#include "engine/physics/RigidbodyComponent.h"

#include "engine/Application.h"

namespace se {
RigidbodyComponent::RigidbodyComponent(const RigidbodyData& data, Entity entity) : data_(data) {
    Application::Get().GetPhysicsManager().AddRigidBody(body_, data, entity);
}

void RigidbodyComponent::Update(float delta_time) {}

void RigidbodyComponent::AddForce(const btVector3& force, const btVector3& point) {
    if (body_) {
        body_->activate(true);
        body_->applyForce(force, point);
    }
}

void RigidbodyComponent::AddImpulse(const btVector3& impulse, const btVector3& point) {
    if (body_) {
        body_->activate(true);
        body_->applyImpulse(impulse, point);
    }
}

void RigidbodyComponent::SetLinearVelocity(const btVector3& velocity) {
    if (body_) {
        body_->activate(true);
        body_->setLinearVelocity(velocity);
    }
}

btVector3 RigidbodyComponent::GetLinearVelocity() const {
    if (body_) {
        return body_->getLinearVelocity();
    }
    return btVector3(0, 0, 0);
}

void RigidbodyComponent::SetAngularFactor(const btVector3& factor) {
    if (body_) {
        body_->setAngularFactor(factor);
    }
}

}  // namespace se