#include "engine/ImGuiLayer.h"

#include <GLFW/glfw3.h>
#include <imgui.h>

#include "engine/Log.h"
#include "examples/imgui_impl_glfw.h"
#include "examples/imgui_impl_opengl3.h"

namespace se {

ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer") {}

ImGuiLayer::~ImGuiLayer() {}

void ImGuiLayer::SetWindow(GLFWwindow* window) {
    window_ = window;
}

void ImGuiLayer::OnAttach() {
    SE_LOG_INFO("ImGuiLayer::OnAttach");

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();

    // When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look
    // identical to regular ones.
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding              = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

    // Setup Platform/Renderer backends
    const char* glsl_version = "#version 330 core";
    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init(glsl_version);
}

void ImGuiLayer::OnDetach() {
    SE_LOG_INFO("ImGuiLayer::OnDetach");

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void ImGuiLayer::OnUpdate(float ts) {
    // ImGui doesn't need per-frame update logic here
}

void ImGuiLayer::OnRender() {
    // Rendering is handled by Begin/End
}

void ImGuiLayer::OnEvent(Event& event) {
    // You can block events here if ImGui wants to capture them
    // For now, we let them pass through
}

void ImGuiLayer::Begin() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::End() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
}

}  // namespace se