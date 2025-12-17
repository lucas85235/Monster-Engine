#pragma once
#include "engine/Layer.h"

namespace FirstGame {
class MainGameLayer : public se::Layer {
   public:
    ~MainGameLayer() override;
    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float ts) override;
    void OnRender() override;
    void OnImGuiRender() override;
};
}  // namespace FirstGame
