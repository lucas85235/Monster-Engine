#include "editor/EditorCamera.h"

#include <algorithm>
#include <cmath>

#include "engine/core/Log.h"

namespace mst {

EditorCamera::EditorCamera() {
    // Position camera at (0, 5, 15) looking towards origin
    camera_.SetPosition({0.0f, 5.0f, 15.0f});
    
    // Camera convention: yaw=0 looks at +X, yaw=-90 looks at -Z
    yaw_ = -90.0f;  // Looking towards -Z (towards origin from positive Z)
    pitch_ = -18.0f;  // Slight downward angle
    
    camera_.SetYaw(yaw_);
    camera_.SetPitch(pitch_);
    
    SE_LOG_INFO("EditorCamera initialized at ({}, {}, {}), yaw={}, pitch={}", 
        0.0f, 5.0f, 15.0f, yaw_, pitch_);
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
    // Calculate direction from camera to point
    Vector3 cameraPos = camera_.GetPosition();
    Vector3 toPoint = point - cameraPos;
    float dist = glm::length(toPoint);
    
    if (dist < 0.01f) {
        // Already at the point, move back
        camera_.SetPosition(point + Vector3(0, 5, 15));
        yaw_ = -90.0f;
        pitch_ = -18.0f;
    } else {
        // Calculate yaw and pitch to look at the point
        // Camera: front.x = cos(yaw) * cos(pitch), front.z = sin(yaw) * cos(pitch)
        Vector3 dir = glm::normalize(toPoint);
        
        // atan2(z, x) gives the angle where cos(angle)=x, sin(angle)=z
        yaw_ = glm::degrees(std::atan2(dir.z, dir.x));
        pitch_ = glm::degrees(std::asin(glm::clamp(dir.y, -1.0f, 1.0f)));
        pitch_ = std::clamp(pitch_, minPitch_, maxPitch_);
    }
    
    camera_.SetYaw(yaw_);
    camera_.SetPitch(pitch_);
    
    // Position camera at focus distance from the target
    Vector3 newFront = camera_.GetFront();
    camera_.SetPosition(point - newFront * focusDistance_);
    
    SE_LOG_INFO("FocusOnPoint: target=({},{},{}), yaw={}, pitch={}", 
        point.x, point.y, point.z, yaw_, pitch_);
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
