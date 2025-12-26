#pragma once

#include "engine/core/Layer.h"

struct GLFWwindow;

namespace se {

class ImGuiLayer : public Layer {
   public:
    ImGuiLayer();
    ~ImGuiLayer() override;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float ts) override;
    void OnRender() override;

    void Begin();  // Start new ImGui frame
    void End();    // Render ImGui draw data

    void SetWindow(GLFWwindow* window);

   private:
    GLFWwindow* window_ = nullptr;
};

}  // namespace se