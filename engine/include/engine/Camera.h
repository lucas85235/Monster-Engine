#pragma once

#include <glm.hpp>

class Camera {
   public:
    // Camera constructor with vectors
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = -90.0f, float pitch = 0.0f);

    // Returns the view matrix calculated using Euler angles and the LookAt matrix
    glm::mat4 getViewMatrix() const;

    // Returns the projection matrix
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

    // Processes input received from a keyboard-like input system
    void processKeyboard(float deltaTime, bool forward, bool back, bool left, bool right, bool upKey, bool downKey);

    // Processes input received from a mouse input system
    void processMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);

    // Processes input received from a mouse scroll-wheel event
    void processMouseScroll(float xoffset, float yoffset);

    // Getters for camera attributes
    glm::vec3 GetPosition() const {
        return position_;
    }
    glm::vec3 GetFront() const {
        return front_;
    }
    glm::vec3 GetUp() const {
        return up_;
    }
    glm::vec3 GetRight() const {
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
    void SetPosition(const glm::vec3& position) {
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
    glm::vec3 position_;
    glm::vec3 front_;
    glm::vec3 up_;
    glm::vec3 right_;
    glm::vec3 world_up_;

    // Euler Angles
    float yaw_;
    float pitch_;

    bool active_ = true;

    // Camera options
    float movement_speed_    = 5.0f;
    float mouse_sensitivity_ = 0.1f;
    float fov_               = 60.0f;
};
