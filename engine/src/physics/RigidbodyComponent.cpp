#include "engine/physics/RigidbodyComponent.h"

#include "engine/Application.h"
#include "engine/ecs/Scene.h"
#include "engine/physics/PhysicsSystem.h"

#include <gtc/quaternion.hpp>
#include <common.hpp>
#include <trigonometric.hpp>

namespace se {
RigidbodyComponent::RigidbodyComponent(const RigidbodyData& data, Entity entity) : data_(data) {
    if (entity.GetScene() && entity.GetScene()->GetPhysicsSystem()) {
        physics_system_ = entity.GetScene()->GetPhysicsSystem();
        body_ = physics_system_->AddRigidBody(entity, data);
    }
}

void RigidbodyComponent::Update(float delta_time) {}

void RigidbodyComponent::AddForce(const btVector3& force, const btVector3& point) {
    if (body_ && physics_system_) {
        std::lock_guard<std::mutex> lock(physics_system_->GetMutex());
        body_->activate(true);
        body_->applyForce(force, point);
    }
}

void RigidbodyComponent::AddImpulse(const btVector3& impulse, const btVector3& point) {
    if (body_ && physics_system_) {
        std::lock_guard<std::mutex> lock(physics_system_->GetMutex());
        body_->activate(true);
        body_->applyImpulse(impulse, point);
    }
}

void RigidbodyComponent::SetLinearVelocity(const btVector3& velocity) {
    if (body_ && physics_system_) {
        std::lock_guard<std::mutex> lock(physics_system_->GetMutex());
        body_->activate(true);
        body_->setLinearVelocity(velocity);
    }
}

btVector3 RigidbodyComponent::GetLinearVelocity() const {
    if (body_ && physics_system_) {
        std::lock_guard<std::mutex> lock(physics_system_->GetMutex());
        return body_->getLinearVelocity();
    }
    return btVector3(0, 0, 0);
}

void RigidbodyComponent::SetAngularFactor(const btVector3& factor) {
    if (body_ && physics_system_) {
        std::lock_guard<std::mutex> lock(physics_system_->GetMutex());
        body_->setAngularFactor(factor);
    }
}

void RigidbodyComponent::SetRotation(const glm::vec3& rotation) {
    if (body_ && physics_system_) {
        std::lock_guard<std::mutex> lock(physics_system_->GetMutex());
        
        btTransform transform = body_->getWorldTransform();
        glm::quat q = glm::quat(glm::radians(rotation));
        transform.setRotation(btQuaternion(q.x, q.y, q.z, q.w));
        
        body_->setWorldTransform(transform);
        if (body_->getMotionState()) {
            body_->getMotionState()->setWorldTransform(transform);
        }
    }
}

}  // namespace se