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
 * Brief: This class is the representation of a character in the game. The character has a collider, rigidbody
 * and other features that simulates how a person behaves in real life.
 *
 */
class Character {
public:
    Character(Entity entity);

private:
    /**
     * Brief: One-line summary of what this function does.
     *
     * Details: Optional extra context (constraints, performance notes, units).
     *
     * @param dt Time step in seconds.
     * @return True if the operation succeeded.
     */

    CharacterSpecs specs_;
    Entity         entity_;
};
} // FirstGame