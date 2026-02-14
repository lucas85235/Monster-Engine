#pragma once

#include <engine/Application.h>
#include <engine/Layer.h>

class UILayer : public se::Layer {
   public:
    UILayer() : Layer("UILayer") {}

    void OnAttach() override {
        SE_LOG_INFO("UILayer attached (native UI placeholder)");
    }

    void OnDetach() override {}
};
