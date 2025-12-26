#pragma once
/**
 * EditorCamera.h - Orbital camera controller for the map editor.
 *
 * Supports orbit (right-click), pan (middle-click), and zoom (scroll) controls.
 * Can focus on selected objects via FocusOnPoint().
 */

#include "Engine.h"
#include "engine/renderer/Camera.h"

namespace mst {

using se::Vector3;
using se::Matrix4;

class EditorCamera {
   public:
    EditorCamera();

    void Update(float deltaTime);

    void OnMouseMove(float dx, float dy, bool leftButton, bool middleButton, bool rightButton);
    void OnMouseScroll(float delta);

    Camera&       GetCamera() { return camera_; }
    const Camera& GetCamera() const { return camera_; }

    void FocusOnPoint(const Vector3& point);
    void SetOrbitDistance(float distance);

    // Movement methods for WASD control
    void MoveForward(float delta);
    void MoveRight(float delta);
    void MoveUp(float delta);

    float GetYaw() const { return yaw_; }
    float GetPitch() const { return pitch_; }
    Vector3 GetPosition() const { return camera_.GetPosition(); }

   private:
    Camera  camera_;
    float   yaw_   = -90.0f;  // Looking towards -Z initially
    float   pitch_ = -15.0f;

    float rotateSpeed_ = 0.2f;
    float panSpeed_    = 0.01f;
    float zoomSpeed_   = 2.0f;
    float moveSpeed_   = 10.0f;
    float focusDistance_ = 10.0f;

    float minPitch_ = -89.0f;
    float maxPitch_ = 89.0f;
};

}  // namespace mst
