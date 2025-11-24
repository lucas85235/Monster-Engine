#pragma once

#include "Engine.h"

class Camera {
   public:
    // Camera constructor with vectors
    Camera(se::Vector3 position = se::Vector3(0.0f, 0.0f, 10.0f), se::Vector3 up = se::Vector3(0.0f, 1.0f, 0.0f), float yaw = -90.0f, float pitch = 0.0f);

    // Returns the view matrix calculated using Euler angles and the LookAt matrix
    se::Matrix4 getViewMatrix() const;

    // Returns the projection matrix
    se::Matrix4 getProjectionMatrix(float aspectRatio) const;

    enum class CameraMovement {
        FORWARD,
        BACKWARD,
        LEFT,
        RIGHT,
        UP,
        DOWN
    };

    // Processes input received from a keyboard-like input system
    void ProcessKeyboard(CameraMovement direction, float deltaTime);

    // Processes input received from a mouse input system
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

    // Processes input received from a mouse scroll-wheel event
    void ProcessMouseScroll(float xoffset, float yoffset);

    // Getters for camera attributes
    se::Vector3 GetPosition() const {
        return position_;
    }
    se::Vector3 GetFront() const {
        return front_;
    }
    se::Vector3 GetUp() const {
        return up_;
    }
    se::Vector3 GetRight() const {
        return right_;
    }
    float GetYaw() const {
        return yaw_;
    }
    float GetPitch() const {
        return pitch_;
    }
    float GetZoom() const {
        return fov_;
    }

    // Setters for camera attributes
    void SetPosition(const se::Vector3& position) {
        position_ = position;
    }

    void SetYaw(float yaw) {
        yaw_ = yaw;
        updateCameraVectors();
    }

    void SetPitch(float pitch) {
        pitch_ = pitch;
        updateCameraVectors();
    }

    void SetZoom(float zoom) {
        fov_ = glm::clamp(zoom, 1.0f, 90.0f);
    }

    void SetActive(bool active) {
        active_ = active;
    }

   private:
    // Calculates the front vector from the Camera's (updated) Euler Angles
    void updateCameraVectors();

   private:
    // Camera Attributes
    se::Vector3 position_;
    se::Vector3 front_;
    se::Vector3 up_;
    se::Vector3 right_;
    se::Vector3 world_up_;

    // Euler Angles
    float yaw_;
    float pitch_;

    bool active_ = true;

    // Camera options
    float movement_speed_    = 5.0f;
    float mouse_sensitivity_ = 0.1f;
    float fov_               = 60.0f;
};
