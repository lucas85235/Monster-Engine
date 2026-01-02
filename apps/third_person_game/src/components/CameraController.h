#pragma once
/**
 * CameraController - Spring arm camera with collision avoidance.
 */

#include "Engine.h"
#include "engine/Camera.h"

namespace se {
struct SpringArmComponent;
struct TransformComponent;
class RigidbodyComponent;
} // namespace se

namespace FirstGame {
using namespace se;



struct SpringArmConfig {
    float targetArmLength = 2.0f;
    float minArmLength    = 0.5f;
    float probeSize       = 0.3f;
    float lerpSpeedIn     = 15.0f;
    float lerpSpeedOut    = 5.0f;
    float initialPitch    = -30.0f;
    float minPitch        = -80.0f;
    float maxPitch        = 80.0f;
    Vector3 socketOffset  = {0.7f, 0.8f, 0.0f};
    bool enableCollision  = true;
};

struct CameraInputConfig {
    float sensitivityX = 0.1f;
    float sensitivityY = 0.1f;
    bool  invertY      = false;
};



class CameraController : public Component {
public:
    CameraController() = default;
    ~CameraController() override;

    // Lifecycle
    void Awake() override;
    void Start() override;
    void LateUpdate(float dt) override;

    // State queries
    Camera* GetCamera() const { return camera_; }

    // Input handling
    void RotateCamera(float deltaX, float deltaY);

    // Configuration
    SpringArmConfig&   GetSpringArmConfig() { return springArmConfig_; }
    CameraInputConfig& GetInputConfig() { return inputConfig_; }


    float GetCurrentYaw() const;
    float GetCurrentPitch() const;
    Vector3 GetForwardDirection() const;
    Vector3 GetRightDirection() const;

private:
    void CreateCamera();
    void SetupSpringArm();
    void CacheComponents();

    void UpdateCameraPosition(float dt);
    Vector3 CalculateCameraDirection() const;
    float CalculateArmLengthWithCollision(const Vector3& targetPos, const Vector3& direction);

    // Component references
    SpringArmComponent*   springArm_ = nullptr;
    TransformComponent*   transform_ = nullptr;
    RigidbodyComponent*   rigidbody_ = nullptr;  // For collision ignore
    Camera*               camera_    = nullptr;

    // Configuration
    SpringArmConfig   springArmConfig_;
    CameraInputConfig inputConfig_;
};

} // namespace FirstGame
