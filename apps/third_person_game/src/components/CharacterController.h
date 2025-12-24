#pragma once

#include "Engine.h"
#include "engine/Camera.h"
#include "engine/input/InputManager.h"

namespace FirstGame {
using namespace se;

/**
 * CharacterController - Component that handles player input and camera control.
 * 
 * This demonstrates the new Component system with automatic lifecycle methods.
 * Awake() is called when added, Start() before first Update, Update() every frame.
 */
class CharacterController : public Component {
public:
    CharacterController() = default;

    ~CharacterController() override = default;

    void Awake() override;

    void Start() override;

    void Update(float dt) override;

    Camera* GetCamera() const { return camera_; }

private:
    void UpdateCamera(float dt);

    void BindInput();

    void UpdateInputs();

    bool    mouseCaptured_ = false;
    Camera* camera_{};
};
} // namespace FirstGame