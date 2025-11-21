#pragma once

#include "engine/Camera.h"
#include "engine/input/InputManager.h"
#include <glm.hpp>

namespace se {

class CameraController {
public:
    explicit CameraController(Camera& camera);

    void OnUpdate(float ts);
    void SetSpeed(float speed) { movementSpeed_ = speed; }
    void SetSensitivity(float sensitivity) { mouseSensitivity_ = sensitivity; }

private:
    Camera& camera_;
    float movementSpeed_ = 5.0f;
    float mouseSensitivity_ = 0.1f;
};

} // namespace se
