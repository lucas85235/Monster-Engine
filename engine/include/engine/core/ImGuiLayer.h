#pragma once

#include "engine/core/Layer.h"

#include <vulkan/vulkan.h>

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
    bool vulkanMode_ = false;
    
    // Vulkan-specific state
    VkDescriptorPool imguiDescriptorPool_ = VK_NULL_HANDLE;
    VkCommandPool imguiCommandPool_ = VK_NULL_HANDLE;
    VkCommandBuffer imguiCommandBuffer_ = VK_NULL_HANDLE;
    bool imguiFontsUploaded_ = false;
};

}  // namespace se