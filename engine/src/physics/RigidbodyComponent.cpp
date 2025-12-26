#include "engine/physics/RigidbodyComponent.h"

#include <common.hpp>
#include <gtc/quaternion.hpp>
#include <trigonometric.hpp>

#include "engine/Application.h"
#include "engine/ecs/Scene.h"
#include "engine/physics/PhysicsSystem.h"

namespace se {
void RigidbodyComponent::Awake() {
    SE_LOG_INFO("RigidbodyComponent::Awake() - mass={}, gravityScale={}, friction={}, freezeX/Y/Z={}/{}/{}",
                data_.mass, data_.gravityScale, data_.material.Friction,
                data_.freezeRotationX, data_.freezeRotationY, data_.freezeRotationZ);

    // Now GetEntity() and GetScene() are available
    if (GetScene() && GetScene()->GetPhysicsSystem()) {
        physics_system_ = GetScene()->GetPhysicsSystem();
        body_           = physics_system_->AddRigidBody(GetEntity(), data_);
        SE_LOG_INFO("RigidbodyComponent::Awake() - Rigidbody added for entity {}", GetEntityID());
    } else {
        SE_LOG_ERROR("RigidbodyComponent::Awake() - PhysicsSystem not available!");
    }
}

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
        glm::quat   q         = glm::quat(glm::radians(rotation));
        transform.setRotation(btQuaternion(q.x, q.y, q.z, q.w));

        body_->setWorldTransform(transform);
        if (body_->getMotionState()) { body_->getMotionState()->setWorldTransform(transform); }
    }
}

void RigidbodyComponent::SetKinematic(bool kinematic) {
    if (body_ && physics_system_) {
        std::lock_guard<std::mutex> lock(physics_system_->GetMutex());

        if (kinematic) {
            body_->setCollisionFlags(body_->getCollisionFlags() |
                                     btCollisionObject::CF_KINEMATIC_OBJECT);
            body_->setActivationState(DISABLE_DEACTIVATION);
            data_.type = RigidbodyType::Kinematic;
        } else {
            body_->setCollisionFlags(body_->getCollisionFlags() &
                                     ~btCollisionObject::CF_KINEMATIC_OBJECT);
            body_->setActivationState(ACTIVE_TAG);
            data_.type = RigidbodyType::Dynamic;
        }
    }
}
} // namespace se