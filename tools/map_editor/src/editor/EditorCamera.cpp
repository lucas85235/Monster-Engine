#include "editor/EditorCamera.h"

#include <algorithm>
#include <cmath>

namespace mst {

EditorCamera::EditorCamera() {
    camera_.SetPosition({0.0f, 5.0f, 15.0f});
    camera_.SetYaw(-90.0f);  // Looking towards -Z
    camera_.SetPitch(-15.0f);
}

void EditorCamera::Update(float deltaTime) {
    // Camera updates are driven by mouse input, no per-frame logic needed
}

void EditorCamera::OnMouseMove(float dx, float dy, bool leftButton, bool middleButton,
                                bool rightButton) {
    if (rightButton) {
        // Rotate camera (look around from current position)
        yaw_ += dx * rotateSpeed_;
        pitch_ -= dy * rotateSpeed_;
        pitch_ = std::clamp(pitch_, minPitch_, maxPitch_);
        
        camera_.SetYaw(yaw_);
        camera_.SetPitch(pitch_);
    }

    if (middleButton) {
        // Pan the camera (move sideways and up/down)
        Vector3 right = camera_.GetRight();
        Vector3 up = camera_.GetUp();
        
        Vector3 position = camera_.GetPosition();
        position -= right * dx * panSpeed_;
        position += up * dy * panSpeed_;
        camera_.SetPosition(position);
    }
}

void EditorCamera::OnMouseScroll(float delta) {
    // Zoom = move forward/backward along view direction
    Vector3 forward = camera_.GetFront();
    Vector3 position = camera_.GetPosition();
    position += forward * delta * zoomSpeed_;
    camera_.SetPosition(position);
}

void EditorCamera::FocusOnPoint(const Vector3& point) {
    // Move camera to look at the point from a reasonable distance
    Vector3 toPoint = point - camera_.GetPosition();
    float dist = glm::length(toPoint);
    
    if (dist < 0.01f) {
        // Already at the point, just set position behind it
        camera_.SetPosition(point - camera_.GetFront() * focusDistance_);
    } else {
        // Calculate yaw and pitch to look at the point
        Vector3 dir = glm::normalize(toPoint);
        
        yaw_ = glm::degrees(std::atan2(dir.x, -dir.z));
        pitch_ = glm::degrees(std::asin(dir.y));
        pitch_ = std::clamp(pitch_, minPitch_, maxPitch_);
        
        camera_.SetYaw(yaw_);
        camera_.SetPitch(pitch_);
        
        // Move camera to focus distance from the point
        camera_.SetPosition(point - camera_.GetFront() * focusDistance_);
    }
}

void EditorCamera::SetOrbitDistance(float distance) {
    focusDistance_ = std::clamp(distance, 1.0f, 100.0f);
}

void EditorCamera::MoveForward(float delta) {
    Vector3 forward = camera_.GetFront();
    Vector3 position = camera_.GetPosition();
    position += forward * delta * moveSpeed_;
    camera_.SetPosition(position);
}

void EditorCamera::MoveRight(float delta) {
    Vector3 right = camera_.GetRight();
    Vector3 position = camera_.GetPosition();
    position += right * delta * moveSpeed_;
    camera_.SetPosition(position);
}

void EditorCamera::MoveUp(float delta) {
    Vector3 position = camera_.GetPosition();
    position.y += delta * moveSpeed_;
    camera_.SetPosition(position);
}

}  // namespace mst
