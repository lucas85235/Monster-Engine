#pragma once

namespace se {

struct PhysicsMaterial {
    float Friction       = 0.5f;
    float Restitution    = 0.0f;
    float LinearDamping  = 0.0f;
    float AngularDamping = 0.05f;

    PhysicsMaterial() = default;
    PhysicsMaterial(float friction, float restitution, float linearDamping = 0.0f,
                    float angularDamping = 0.05f)
        : Friction(friction),
          Restitution(restitution),
          LinearDamping(linearDamping),
          AngularDamping(angularDamping) {}
};

}  // namespace se
