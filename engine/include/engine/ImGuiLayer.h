#pragma once

#include <string>

#include "engine/Layer.h"

struct GLFWwindow;

namespace se {

class ImGuiLayer : public Layer {
   public:
    ImGuiLayer();
    explicit ImGuiLayer(const std::string& appName);
    ~ImGuiLayer() override;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float ts) override;
    void OnRender() override;

    void Begin();  // Start new ImGui frame
    void End();    // Render ImGui draw data
    bool IsFrameActive() const;

    void SetWindow(GLFWwindow* window);
    void SetAppName(const std::string& appName);

   private:
    GLFWwindow* window_ = nullptr;
    bool ownsWindow_ = false;
    bool initialized_ = false;
    bool frameActive_ = false;
    std::string appName_ = "default";
    std::string iniFilePath_;
};

}  // namespace se
