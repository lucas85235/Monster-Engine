#include "editor/EditorCamera.h"

#include <algorithm>
#include <cmath>

namespace mst {

EditorCamera::EditorCamera() {
    camera_.SetPosition({0.0f, 5.0f, 10.0f});
    UpdateCameraTransform();
}

void EditorCamera::Update(float deltaTime) {
    // Camera updates are driven by mouse input, no per-frame logic needed
}

void EditorCamera::OnMouseMove(float dx, float dy, bool leftButton, bool middleButton,
                                bool rightButton) {
    if (rightButton) {
        // Orbit around focus point
        yaw_ += dx * orbitSpeed_;
        pitch_ -= dy * orbitSpeed_;
        pitch_ = std::clamp(pitch_, minPitch_, maxPitch_);
        UpdateCameraTransform();
    }

    if (middleButton) {
        // Pan the camera
        Vector3 right = camera_.GetRight();
        Vector3 up    = camera_.GetUp();

        float panFactor = orbitDistance_ * panSpeed_;
        focusPoint_ -= right * dx * panFactor;
        focusPoint_ += up * dy * panFactor;
        UpdateCameraTransform();
    }
}

void EditorCamera::OnMouseScroll(float delta) {
    orbitDistance_ -= delta * zoomSpeed_;
    orbitDistance_ = std::clamp(orbitDistance_, minDistance_, maxDistance_);
    UpdateCameraTransform();
}

void EditorCamera::FocusOnPoint(const Vector3& point) {
    focusPoint_ = point;
    UpdateCameraTransform();
}

void EditorCamera::SetOrbitDistance(float distance) {
    orbitDistance_ = std::clamp(distance, minDistance_, maxDistance_);
    UpdateCameraTransform();
}

void EditorCamera::UpdateCameraTransform() {
    float yawRad   = glm::radians(yaw_);
    float pitchRad = glm::radians(pitch_);

    Vector3 direction;
    direction.x = std::cos(pitchRad) * std::sin(yawRad);
    direction.y = std::sin(pitchRad);
    direction.z = std::cos(pitchRad) * std::cos(yawRad);

    Vector3 position = focusPoint_ + direction * orbitDistance_;

    camera_.SetPosition(position);
    
    // Calculate yaw/pitch to look at focus point
    Vector3 toTarget = glm::normalize(focusPoint_ - position);
    float cameraYaw = glm::degrees(std::atan2(toTarget.x, toTarget.z));
    float cameraPitch = glm::degrees(std::asin(toTarget.y));
    
    camera_.SetYaw(cameraYaw - 90.0f);  // Camera yaw has -90 offset
    camera_.SetPitch(cameraPitch);
}

}  // namespace mst
