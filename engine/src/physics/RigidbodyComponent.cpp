#include "engine/physics/RigidbodyComponent.h"

#include "engine/Application.h"

namespace se {
RigidbodyComponent::RigidbodyComponent(const RigidbodyData& data) : data_(data) {
    Application::Get().GetPhysicsManager().AddRigidBody(*body_);
}

void RigidbodyComponent::Update(float delta_time) {}

}  // namespace se