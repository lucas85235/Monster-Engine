#pragma once
/**
 * CharacterController - Translates user input to Character and Camera actions.
 */

#include "Engine.h"
#include "engine/input/InputManager.h"

namespace FirstGame {
using namespace se;

class Character;
class CameraController;

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

class CharacterController : public Component {
public:
    CharacterController() = default;

    ~CharacterController() override = default;

    // Lifecycle
    void Awake() override;

    void Start() override;

    void Update(float dt) override;
    void OnDestroy() override;

    // State queries

    // Configuration queries

private:
    void BindInputs();
    void CacheComponents();
    void ProcessInput(float dt);
    void HandleMouseToggle();
    void SetMouseCaptured(bool captured);
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

    const std::string inputMapName_ = "third_person_game.character";
};
} // namespace FirstGame
