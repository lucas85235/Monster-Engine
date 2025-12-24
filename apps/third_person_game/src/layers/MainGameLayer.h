#pragma once
#include "../Character.h"
#include "engine/Camera.h"
#include "engine/Layer.h"
#include "engine/ecs/Scene.h"
#include "engine/renderer/Material.h"

namespace FirstGame {
class MainGameLayer : public se::Layer {
   public:
    ~MainGameLayer() override;

    void OnAttach() override;

    void OnDetach() override;

    void OnUpdate(float ts) override;

    void OnRender() override;

    void OnImGuiRender() override;

   private:
    Ref<Character> character_;
    Scope<Scene>   scene_;
    Camera         camera_;
    Ref<Material>  material_;
    bool           mouseCaptured_ = false;
};
}  // namespace FirstGame