#include <engine/Camera.h>

#include "ext/matrix_clip_space.hpp"
#include "ext/matrix_transform.hpp"

Camera::Camera(glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : position_(position), world_up_(up), yaw_(yaw), pitch_(pitch), front_(glm::vec3(0.0f, 0.0f, -1.0f)) {
    updateCameraVectors();
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(position_, position_ + front_, up_);
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(fov_), aspectRatio, 0.1f, 100.0f);
}

void Camera::ProcessKeyboard(CameraMovement direction, float deltaTime) {
    float velocity = movement_speed_ * deltaTime;
    if (direction == CameraMovement::FORWARD) position_ += front_ * velocity;
    if (direction == CameraMovement::BACKWARD) position_ -= front_ * velocity;
    if (direction == CameraMovement::LEFT) position_ -= right_ * velocity;
    if (direction == CameraMovement::RIGHT) position_ += right_ * velocity;
    if (direction == CameraMovement::UP) position_ += world_up_ * velocity;
    if (direction == CameraMovement::DOWN) position_ -= world_up_ * velocity;
}

void Camera::ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch) {
    if (!active_) return;
    xoffset *= mouse_sensitivity_;
    yoffset *= mouse_sensitivity_;

    yaw_ += xoffset;
    pitch_ += yoffset;

    if (constrainPitch) {
        if (pitch_ > 89.0f) pitch_ = 89.0f;
        if (pitch_ < -89.0f) pitch_ = -89.0f;
    }

    updateCameraVectors();
}

void Camera::ProcessMouseScroll(float xoffset, float yoffset) {
    fov_ -= yoffset;
    if (fov_ < 1.0f) fov_ = 1.0f;
    if (fov_ > 90.0f) fov_ = 90.0f;

    SE_LOG_INFO("Camera Fov: {}", fov_);
}

void Camera::updateCameraVectors() {
    // calculate the new Front vector
    glm::vec3 front;
    front.x = cos(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    front.y = sin(glm::radians(pitch_));
    front.z = sin(glm::radians(yaw_)) * cos(glm::radians(pitch_));
    front_  = glm::normalize(front);
    // also re-calculate Right and Up vector
    right_ = glm::normalize(glm::cross(front_, world_up_));  // normalize the vectors
    up_    = glm::normalize(glm::cross(right_, front_));
}
