#pragma once

#include "engine/gameplay/Controller.h"
#include "engine/Camera.h"

#include <string>

namespace se {

class Character;
class PhysicsSystem;
struct TransformComponent;

struct PlayerInputConfig {
    std::string moveForward = "MoveForward";
    std::string moveRight   = "MoveRight";
    std::string jump        = "Jump";
    std::string sprint      = "Sprint";
    std::string lookX       = "CameraRotateX";
    std::string lookY       = "CameraRotateY";
    std::string toggleMouse = "ToggleMouse";
};

struct CameraConfig {
    float targetArmLength = 3.0f;
    float minArmLength    = 0.5f;
    Vector3 socketOffset{0.7f, 0.8f, 0.0f};
    float probeRadius     = 0.3f;
    bool  doCollisionTest = true;
    float lagSpeedIn      = 15.0f;
    float lagSpeedOut     = 5.0f;
    float initialPitch    = -20.0f;
    float minPitch        = -80.0f;
    float maxPitch        = 80.0f;
    float sensitivityX    = 0.1f;
    float sensitivityY    = 0.1f;
    bool  invertY         = false;
};

class PlayerController : public Controller {
   public:
    PlayerController()           = default;
    ~PlayerController() override = default;

    void Awake() override;
    void Start() override;
    void Update(float dt) override;
    void LateUpdate(float dt) override;

    bool IsPlayerController() const override { return true; }

    // Input control
    void SetInputEnabled(bool enabled) { inputEnabled_ = enabled; }
    bool IsInputEnabled() const { return inputEnabled_; }

    // Mouse capture
    void SetMouseCaptured(bool captured);
    bool IsMouseCaptured() const { return mouseCaptured_; }
    void ToggleMouseCapture();

    // Camera access
    Camera* GetCamera() const { return camera_; }
    void    SetCamera(Camera* camera) { camera_ = camera; }

    // Configuration access
    PlayerInputConfig& GetInputConfig() { return inputConfig_; }
    CameraConfig&      GetCameraConfig() { return cameraConfig_; }
    const CameraConfig& GetCameraConfig() const { return cameraConfig_; }

    // Camera queries
    float   GetCurrentYaw() const { return controlRotation_.y; }
    float   GetCurrentPitch() const { return controlRotation_.x; }
    Vector3 GetForwardDirection() const;
    Vector3 GetRightDirection() const;

   protected:
    void OnPossess(Pawn* pawn) override;
    void OnUnpossess() override;

   private:
    void BindInputs();
    void ProcessInput(float dt);
    void ProcessMovementInput();
    void ProcessCameraInput();
    void UpdateCamera(float dt);
    float CalculateArmLengthWithCollision(const Vector3& targetPos, const Vector3& direction);

    // Camera
    Camera*         camera_         = nullptr;
    PhysicsSystem*  physicsSystem_  = nullptr;
    float           currentArmLength_ = 3.0f;

    // Configuration
    PlayerInputConfig inputConfig_;
    CameraConfig      cameraConfig_;

    // State
    bool inputEnabled_  = true;
    bool mouseCaptured_ = false;
};

}  // namespace se
