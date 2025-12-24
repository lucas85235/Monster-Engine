#pragma once
#include "engine/physics/Collider.h"
#include "engine/physics/RigidbodyComponent.h"

namespace FirstGame {
using namespace se;

struct CharacterSpecs {
    float acceleration_       = 3.4f;
    float max_movement_speed_ = 5.0f;
    float max_rotation_speed_ = 180.0f;
    float jump_force_         = 3.0f;
    float character_height_   = 1.0f;
    float character_radius_   = 0.5f;
};

/**
 * Character component - represents a player character with physics.
 * Uses Component lifecycle methods (Awake/Start/Update called automatically).
 */
class Character : public Component {
public:
    ~Character() override = default;

    void Awake() override;

    void Start() override;

    void Update(float dt) override;

    const CharacterSpecs& GetSpecs() const { return specs_; }

private:
    CharacterSpecs specs_;
};
} // namespace FirstGame
