#include "engine/gameplay/Controller.h"
#include "engine/gameplay/Pawn.h"
#include "engine/Log.h"

namespace se {

void Controller::Start() {
    // Auto-possess pawn if on same entity
    Pawn* pawn = GetEntity().FindComponent<Pawn>();
    if (pawn && !pawn_) {
        Possess(pawn);
    }
}

void Controller::OnDestroy() {
    Unpossess();
}

void Controller::Possess(Pawn* pawn) {
    if (!pawn) {
        Unpossess();
        return;
    }

    if (pawn_ == pawn) return;

    // Unpossess current pawn first
    if (pawn_) {
        Unpossess();
    }

    pawn_ = pawn;
    pawn_->SetController(this);
    
    OnPossess(pawn);
    SE_LOG_DEBUG("[Controller] Entity {} possessed pawn {}", GetEntityID(), pawn->GetEntityID());
}

void Controller::Unpossess() {
    if (!pawn_) return;

    Pawn* oldPawn = pawn_;
    pawn_ = nullptr;
    
    oldPawn->SetController(nullptr);
    
    OnUnpossess();
    SE_LOG_DEBUG("[Controller] Entity {} unpossessed pawn", GetEntityID());
}

void Controller::SetControlRotation(const Vector3& rotation) {
    controlRotation_ = rotation;
    
    if (pawn_) {
        pawn_->SetControlRotation(rotation);
    }
}

void Controller::AddControlRotation(float yaw, float pitch) {
    controlRotation_.y += yaw;
    controlRotation_.x += pitch;
    
    // Clamp pitch
    controlRotation_.x = glm::clamp(controlRotation_.x, -89.0f, 89.0f);
    
    // Normalize yaw
    while (controlRotation_.y > 180.0f) controlRotation_.y -= 360.0f;
    while (controlRotation_.y < -180.0f) controlRotation_.y += 360.0f;
    
    if (pawn_) {
        pawn_->SetControlRotation(controlRotation_);
    }
}

}  // namespace se
