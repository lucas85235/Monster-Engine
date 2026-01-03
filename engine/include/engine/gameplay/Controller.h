#pragma once

#include "engine/ecs/Component.h"
#include "engine/ecs/Entity.h"

#include <glm.hpp>

namespace se {

class Pawn;

class Controller : public Component {
   public:
    Controller()           = default;
    ~Controller() override = default;

    void Start() override;
    void OnDestroy() override;

    // Possession API
    virtual void Possess(Pawn* pawn);
    virtual void Unpossess();
    
    Pawn* GetPawn() const { return pawn_; }

    template <typename T>
    T* GetPawn() const {
        return dynamic_cast<T*>(pawn_);
    }

    bool HasPawn() const { return pawn_ != nullptr; }

    // Control rotation (camera/aiming direction)
    void          SetControlRotation(const Vector3& rotation);
    const Vector3& GetControlRotation() const { return controlRotation_; }
    void          AddControlRotation(float yaw, float pitch);

    // Controller type queries (override in subclasses)
    virtual bool IsPlayerController() const { return false; }
    virtual bool IsAIController() const { return false; }

   protected:
    // Called when possession changes (override in subclasses)
    virtual void OnPossess(Pawn* pawn) {}
    virtual void OnUnpossess() {}

    Pawn*   pawn_ = nullptr;
    Vector3 controlRotation_{0.0f};  // Yaw, Pitch, Roll
};

}  // namespace se
