#include "engine/gameplay/Pawn.h"
#include "engine/gameplay/Controller.h"
#include "engine/Log.h"

namespace se {

void Pawn::Update(float dt) {
    // Base pawn doesn't do anything special in update
    // Subclasses (Character) will process movement
}

void Pawn::SetController(Controller* controller) {
    if (controller_ == controller) return;

    Controller* oldController = controller_;
    
    if (oldController) {
        controller_ = nullptr;
        OnUnpossessed();
    }

    if (controller) {
        controller_ = controller;
        OnPossessed(controller);
    }
}

bool Pawn::IsPlayerControlled() const {
    return controller_ && controller_->IsPlayerController();
}

void Pawn::AddMovementInput(const Vector3& direction, float scale) {
    pendingMovementInput_ += direction * scale;
}

void Pawn::AddControllerYawInput(float value) {
    pendingYawInput_ += value;
}

void Pawn::AddControllerPitchInput(float value) {
    pendingPitchInput_ += value;
}

Vector3 Pawn::ConsumeMovementInput() {
    Vector3 input = pendingMovementInput_;
    pendingMovementInput_ = Vector3{0.0f};
    return input;
}

float Pawn::ConsumeYawInput() {
    float input = pendingYawInput_;
    pendingYawInput_ = 0.0f;
    return input;
}

float Pawn::ConsumePitchInput() {
    float input = pendingPitchInput_;
    pendingPitchInput_ = 0.0f;
    return input;
}

void Pawn::SetControlRotation(const Vector3& rotation) {
    controlRotation_ = rotation;
}

void Pawn::OnPossessed(Controller* controller) {
    SE_LOG_DEBUG("[Pawn] Entity {} possessed by controller", GetEntityID());
}

void Pawn::OnUnpossessed() {
    SE_LOG_DEBUG("[Pawn] Entity {} unpossessed", GetEntityID());
}

}  // namespace se
