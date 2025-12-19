#include "engine/renderer/CameraController.h"
#include "engine/input/InputManager.h"

namespace se {

CameraController::CameraController(Camera& camera) : camera_(camera) {
    // Setup default bindings if not already set? 
    // Ideally the app sets bindings, but we can have defaults.
    // For now, we assume bindings "MoveForward", "MoveRight", "MoveUp", "LookX", "LookY" exist or we check raw keys if we want fallback.
    // But the plan was to use InputManager.
    
    // Let's assume the user of this class ensures bindings are set, OR we check specific keys as fallback?
    // No, let's stick to the plan: use InputManager.
    // But to make it "plug and play" for the existing app, maybe we should bind defaults here if they don't exist?
    // Better: The AppLayer should set up bindings.
}

void CameraController::OnUpdate(float ts) {
    auto& input = InputManager::Get();

    // Movement
    float velocity = movementSpeed_ * ts;
    
    float moveForward = input.GetAxis("MoveForward"); // W - S
    float moveRight = input.GetAxis("MoveRight");     // D - A
    float moveUp = input.GetAxis("MoveUp");           // Space - Ctrl

    if (moveForward != 0.0f) {
        camera_.ProcessKeyboard(Camera::CameraMovement::FORWARD, moveForward * velocity);
    }
    if (moveRight != 0.0f) {
        camera_.ProcessKeyboard(Camera::CameraMovement::RIGHT, moveRight * velocity);
    }
    if (moveUp != 0.0f) {
        camera_.ProcessKeyboard(Camera::CameraMovement::UP, moveUp * velocity);
    }

    // Mouse Look
    // We need to handle mouse capture state? 
    // The InputHandler did "setCursorModeFromString".
    // We might need a way to toggle cursor mode in InputManager or Window.
    // For now, let's assume the app handles cursor mode.
    
    float lookX = input.GetAxis("LookX");
    float lookY = input.GetAxis("LookY");

    if (lookX != 0.0f || lookY != 0.0f) {
        camera_.ProcessMouseMovement(lookX, lookY);
    }
}

} // namespace se
