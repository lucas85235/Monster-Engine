#pragma once

#include "Engine.h"
#include "engine/Camera.h"

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

    float GetOrbitDistance() const { return orbitDistance_; }
    float GetYaw() const { return yaw_; }
    float GetPitch() const { return pitch_; }

   private:
    void UpdateCameraTransform();

    Camera  camera_;
    Vector3 focusPoint_{0.0f, 0.0f, 0.0f};
    float   orbitDistance_ = 15.0f;
    float   yaw_           = 45.0f;
    float   pitch_         = -30.0f;

    float orbitSpeed_ = 0.3f;
    float panSpeed_   = 0.02f;
    float zoomSpeed_  = 1.5f;

    float minDistance_ = 1.0f;
    float maxDistance_ = 100.0f;
    float minPitch_    = -89.0f;
    float maxPitch_    = 89.0f;
};

}  // namespace mst
