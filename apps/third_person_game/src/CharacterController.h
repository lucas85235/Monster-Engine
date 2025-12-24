#pragma once

#include "Engine.h"
#include "apps/third_person_game/src/Character.h"
#include "engine/Camera.h"
#include "engine/ecs/Scene.h"
#include "engine/input/InputManager.h"
using namespace se;

namespace FirstGame {
class CharacterController : public Component {
   public:
    CharacterController() = default;
    CharacterController(Character* character, Scene* scene);
    ~CharacterController() override;

   private:
    void Update(float ts) override;
    void UpdateCamera();
    void BindInput();
    void UpdateInputs();

   private:
    bool       mouseCaptured_;
    Character* character_;
    Camera     camera_;
    Scene*     scene_;
};
}  // namespace FirstGame