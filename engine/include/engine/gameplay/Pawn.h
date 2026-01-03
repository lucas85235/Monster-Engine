#pragma once

#include "engine/ecs/Component.h"
#include "engine/ecs/Entity.h"

#include <glm.hpp>

namespace se {

class Controller;

class Pawn : public Component {
   public:
    Pawn()           = default;
    ~Pawn() override = default;

    void Update(float dt) override;

    // Possession API
    void        SetController(Controller* controller);
    Controller* GetController() const { return controller_; }

    template <typename T>
    T* GetController() const {
        return dynamic_cast<T*>(controller_);
    }

    bool IsControlled() const { return controller_ != nullptr; }
    bool IsPlayerControlled() const;

    // Movement input (called by Controller)
    void AddMovementInput(const Vector3& direction, float scale = 1.0f);
    void AddControllerYawInput(float value);
    void AddControllerPitchInput(float value);

    // Consume and clear movement input (called during movement processing)
    Vector3 ConsumeMovementInput();
    float   ConsumeYawInput();
    float   ConsumePitchInput();

    // Control rotation (set by Controller, used for camera-relative movement)
    void          SetControlRotation(const Vector3& rotation);
    const Vector3& GetControlRotation() const { return controlRotation_; }

    // Events (override in subclasses)
    virtual void OnPossessed(Controller* controller);
    virtual void OnUnpossessed();

   protected:
    Controller* controller_ = nullptr;

    // Accumulated input (reset each frame after consumption)
    Vector3 pendingMovementInput_{0.0f};
    float   pendingYawInput_   = 0.0f;
    float   pendingPitchInput_ = 0.0f;

    // Control rotation (yaw, pitch, roll from controller)
    Vector3 controlRotation_{0.0f};
};

}  // namespace se
