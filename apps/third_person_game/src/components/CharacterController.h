#pragma once
/**
 * CharacterController.h - Player input controller.
 *
 * This component manages:
 * - Input binding and processing
 * - Translates input to Character movement commands
 * - Delegates camera input to CameraController
 *
 * Requires: Character, CameraController on same entity
 */

#include "Engine.h"
#include "engine/input/InputManager.h"

namespace FirstGame {
using namespace se;

class Character;
class CameraController;

// ============================================================================
// Input Configuration
// ============================================================================

struct MovementInputConfig {
    std::string moveForward = "MoveForward";
    std::string moveRight   = "MoveRight";
    std::string jump        = "Jump";
    std::string sprint      = "Sprint";
};

struct CameraInputBindings {
    std::string toggleMouse   = "ToggleMouse";
    std::string cameraRotateX = "CameraRotateX";
    std::string cameraRotateY = "CameraRotateY";
};

// ============================================================================
// CharacterController Component
// ============================================================================

class CharacterController : public Component {
public:
    CharacterController() = default;
    ~CharacterController() override = default;

    // Lifecycle
    void Awake() override;
    void Start() override;
    void Update(float dt) override;

    // === State ===
    bool IsMouseCaptured() const { return mouseCaptured_; }

    // === Configuration ===
    MovementInputConfig&  GetMovementConfig() { return movementConfig_; }
    CameraInputBindings&  GetCameraBindings() { return cameraBindings_; }

private:
    void BindInputs();
    void CacheComponents();

    void ProcessInput();
    void HandleMouseToggle();
    void ProcessMovementInput();
    void ProcessCameraInput();

    // Component references
    Character*        character_        = nullptr;
    CameraController* cameraController_ = nullptr;

    // Configuration
    MovementInputConfig movementConfig_;
    CameraInputBindings cameraBindings_;

    // State
    bool mouseCaptured_ = false;
};

} // namespace FirstGame