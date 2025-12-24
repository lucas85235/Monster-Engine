#pragma once

#include "BulletDynamics/Dynamics/btRigidBody.h"
#include "engine/ecs/Component.h"
#include "engine/physics/PhysicsMaterial.h"

namespace se {
enum class RigidbodyType { Dynamic, Static, Kinematic };

struct RigidbodyData {
    float           mass            = 1.0f;
    float           gravityScale    = 1.0f;
    RigidbodyType   type            = RigidbodyType::Dynamic;
    PhysicsMaterial material        = {};
    bool            freezeRotationX = false;
    bool            freezeRotationY = false;
    bool            freezeRotationZ = false;
};

class RigidbodyComponent : public Component {
public:
    RigidbodyComponent() = default;
    RigidbodyComponent(const RigidbodyData& data);
    
    void Awake() override;

    btRigidBody* GetRigidbody() const {
        return body_;
    }

    const RigidbodyData& GetData() const {
        return data_;
    }

    void AddForce(const btVector3& force, const btVector3& point);

    void AddImpulse(const btVector3& impulse, const btVector3& point);

    void SetLinearVelocity(const btVector3& velocity);

    btVector3 GetLinearVelocity() const;

    void SetAngularFactor(const btVector3& factor);

    void SetRotation(const glm::vec3& rotation);

    void SetKinematic(bool kinematic);

    bool IsKinematic() const {
        return data_.type == RigidbodyType::Kinematic;
    }

private:
    RigidbodyData        data_;
    btRigidBody*         body_           = nullptr;
    class PhysicsSystem* physics_system_ = nullptr;
};
} // namespace se