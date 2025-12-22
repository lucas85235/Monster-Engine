#include "engine/core/ImGuiLayer.h"

#include <GLFW/glfw3.h>
#include <imgui.h>

#include "engine/core/Application.h"
#include "engine/core/Log.h"
#include "engine/renderer/GraphicsContext.h"
#include "examples/imgui_impl_glfw.h"
#include "examples/imgui_impl_opengl3.h"
#include "examples/imgui_impl_vulkan.h"
#include "engine/rhi/vulkan/vulkan_device.h"

namespace se {

static RHI::IDevice* GetDevice() {
    auto& app     = Application::Get();
    auto* context = app.GetWindow().GetContext();
    return context ? context->GetDevice() : nullptr;
}

static void CheckVkResult(VkResult err) {
    if (err != VK_SUCCESS) {
        SE_LOG_ERROR("[ImGui Vulkan] Error: VkResult = {}", static_cast<int>(err));
    }
}

ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer") {}

ImGuiLayer::~ImGuiLayer() {}

void ImGuiLayer::SetWindow(GLFWwindow* window) {
    window_ = window;
}

void ImGuiLayer::OnAttach() {
    SE_LOG_INFO("ImGuiLayer::OnAttach");

    auto* device = GetDevice();
    vulkanMode_ = (device && device->GetAPI() == RHI::API::Vulkan);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // Disable viewports for Vulkan (requires additional platform work)
    if (!vulkanMode_) {
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    }
    
    // Disable mouse cursor changes on Vulkan - glfwSetCursor is slow on Linux/X11
    if (vulkanMode_) {
        io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;
    }

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look
    // identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding              = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    if (vulkanMode_) {
        // Vulkan backend initialization
        SE_LOG_INFO("ImGuiLayer: Initializing Vulkan backend");
        
        auto* vkDevice = static_cast<RHI::VulkanDevice*>(device);
        VkDevice logicalDevice = vkDevice->GetVkDevice();
        
        // Create a dedicated descriptor pool for ImGui (exactly as in the official example)
        VkDescriptorPoolSize poolSizes[] = {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };
        
        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        poolInfo.maxSets = 1000 * 11;  // 1000 * number of pool sizes
        poolInfo.poolSizeCount = 11;
        poolInfo.pPoolSizes = poolSizes;
        
        VkResult result = vkCreateDescriptorPool(logicalDevice, &poolInfo, nullptr, &imguiDescriptorPool_);
        if (result != VK_SUCCESS) {
            SE_LOG_ERROR("ImGuiLayer: Failed to create ImGui descriptor pool!");
            vulkanMode_ = false;
            return;
        }
        
        // Create dedicated command pool for font upload
        VkCommandPoolCreateInfo cmdPoolInfo = {};
        cmdPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        cmdPoolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        cmdPoolInfo.queueFamilyIndex = vkDevice->GetGraphicsQueueFamily();
        
        result = vkCreateCommandPool(logicalDevice, &cmdPoolInfo, nullptr, &imguiCommandPool_);
        if (result != VK_SUCCESS) {
            SE_LOG_ERROR("ImGuiLayer: Failed to create command pool!");
            vkDestroyDescriptorPool(logicalDevice, imguiDescriptorPool_, nullptr);
            imguiDescriptorPool_ = VK_NULL_HANDLE;
            vulkanMode_ = false;
            return;
        }
        
        // Allocate command buffer
        VkCommandBufferAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = imguiCommandPool_;
        allocInfo.commandBufferCount = 1;
        
        result = vkAllocateCommandBuffers(logicalDevice, &allocInfo, &imguiCommandBuffer_);
        if (result != VK_SUCCESS) {
            SE_LOG_ERROR("ImGuiLayer: Failed to allocate command buffer!");
            vkDestroyCommandPool(logicalDevice, imguiCommandPool_, nullptr);
            vkDestroyDescriptorPool(logicalDevice, imguiDescriptorPool_, nullptr);
            imguiCommandPool_ = VK_NULL_HANDLE;
            imguiDescriptorPool_ = VK_NULL_HANDLE;
            vulkanMode_ = false;
            return;
        }
        
        // Initialize GLFW for Vulkan
        ImGui_ImplGlfw_InitForVulkan(window_, true);
        
        // Setup Vulkan init info
        ImGui_ImplVulkan_InitInfo initInfo = {};
        initInfo.Instance = vkDevice->GetVkInstance();
        initInfo.PhysicalDevice = vkDevice->GetVkPhysicalDevice();
        initInfo.Device = logicalDevice;
        initInfo.QueueFamily = vkDevice->GetGraphicsQueueFamily();
        initInfo.Queue = vkDevice->GetVkGraphicsQueue();
        initInfo.PipelineCache = VK_NULL_HANDLE;
        initInfo.DescriptorPool = imguiDescriptorPool_;
        initInfo.MinImageCount = vkDevice->GetMinImageCount();
        initInfo.ImageCount = vkDevice->GetImageCount();
        initInfo.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        initInfo.Allocator = nullptr;
        initInfo.CheckVkResultFn = CheckVkResult;
        
        if (!ImGui_ImplVulkan_Init(&initInfo, vkDevice->GetVkRenderPass())) {
            SE_LOG_ERROR("ImGuiLayer: Failed to initialize ImGui_ImplVulkan!");
            vkDestroyCommandPool(logicalDevice, imguiCommandPool_, nullptr);
            vkDestroyDescriptorPool(logicalDevice, imguiDescriptorPool_, nullptr);
            imguiCommandPool_ = VK_NULL_HANDLE;
            imguiDescriptorPool_ = VK_NULL_HANDLE;
            vulkanMode_ = false;
            return;
        }
        
        // Upload fonts immediately (following official example pattern)
        SE_LOG_INFO("ImGuiLayer: Uploading fonts...");
        
        result = vkResetCommandPool(logicalDevice, imguiCommandPool_, 0);
        CheckVkResult(result);
        
        VkCommandBufferBeginInfo beginInfo = {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
        
        result = vkBeginCommandBuffer(imguiCommandBuffer_, &beginInfo);
        CheckVkResult(result);
        
        ImGui_ImplVulkan_CreateFontsTexture(imguiCommandBuffer_);
        
        VkSubmitInfo submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &imguiCommandBuffer_;
        
        result = vkEndCommandBuffer(imguiCommandBuffer_);
        CheckVkResult(result);
        
        result = vkQueueSubmit(vkDevice->GetVkGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        CheckVkResult(result);
        
        result = vkDeviceWaitIdle(logicalDevice);
        CheckVkResult(result);
        
        ImGui_ImplVulkan_DestroyFontUploadObjects();
        
        imguiFontsUploaded_ = true;
        SE_LOG_INFO("ImGuiLayer: Vulkan backend initialized successfully");
    } else {
        // OpenGL backend initialization
        const char* glsl_version = "#version 330 core";
        ImGui_ImplGlfw_InitForOpenGL(window_, true);
        ImGui_ImplOpenGL3_Init(glsl_version);
    }
}

void ImGuiLayer::OnDetach() {
    SE_LOG_INFO("ImGuiLayer::OnDetach");

    if (vulkanMode_) {
        auto* device = GetDevice();
        if (device) {
            auto* vkDevice = static_cast<RHI::VulkanDevice*>(device);
            VkDevice logicalDevice = vkDevice->GetVkDevice();
            vkDeviceWaitIdle(logicalDevice);
            
            ImGui_ImplVulkan_Shutdown();
            
            if (imguiCommandPool_ != VK_NULL_HANDLE) {
                vkDestroyCommandPool(logicalDevice, imguiCommandPool_, nullptr);
                imguiCommandPool_ = VK_NULL_HANDLE;
            }
            if (imguiDescriptorPool_ != VK_NULL_HANDLE) {
                vkDestroyDescriptorPool(logicalDevice, imguiDescriptorPool_, nullptr);
                imguiDescriptorPool_ = VK_NULL_HANDLE;
            }
        }
    } else {
        ImGui_ImplOpenGL3_Shutdown();
    }
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::OnUpdate(float ts) {
    // ImGui doesn't need per-frame update logic here
}

void ImGuiLayer::OnRender() {
    // Rendering is handled by Begin/End
}

void ImGuiLayer::Begin() {
    if (vulkanMode_) {
        ImGui_ImplVulkan_NewFrame();
    } else {
        ImGui_ImplOpenGL3_NewFrame();
    }
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::End() {
    ImGui::Render();
    
    if (vulkanMode_) {
        auto* device = GetDevice();
        if (device) {
            auto* vkDevice = static_cast<RHI::VulkanDevice*>(device);
            VkCommandBuffer cmdBuffer = vkDevice->GetCurrentCommandBuffer();
            if (cmdBuffer != VK_NULL_HANDLE) {
                ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmdBuffer);
            }
        }
    } else {
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            GLFWwindow* backup_current_context = glfwGetCurrentContext();
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
            glfwMakeContextCurrent(backup_current_context);
        }
    }
}

}  // namespace se