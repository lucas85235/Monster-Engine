#pragma once

#include "engine/Layer.h"

namespace se::ui {

class NativeUiLayer final : public Layer {
   public:
    NativeUiLayer();
    ~NativeUiLayer() override = default;

    void OnUpdate(float ts) override;
    void OnRender() override;
};

}  // namespace se::ui
