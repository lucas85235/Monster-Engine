#include "engine/ImGuiLayer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include "engine/Log.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

namespace se {

ImGuiLayer::ImGuiLayer() : Layer("ImGuiLayer") {}

ImGuiLayer::ImGuiLayer(const std::string& appName) : Layer("ImGuiLayer"), appName_(appName) {}

ImGuiLayer::~ImGuiLayer() {}

void ImGuiLayer::SetWindow(GLFWwindow* window) {
    window_ = window;
}

void ImGuiLayer::SetAppName(const std::string& appName) {
    appName_ = appName;
}

void ImGuiLayer::OnAttach() {
    if (initialized_) return;

    // Create a dedicated OpenGL window when the main renderer uses GLFW_NO_API (Filament Metal/Vulkan).
    if (!window_) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#if defined(__APPLE__)
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
        window_     = glfwCreateWindow(540, 920, "Monster Engine - Render Controls", nullptr, nullptr);
        ownsWindow_ = (window_ != nullptr);
    }

    if (!window_) {
        SE_LOG_ERROR("ImGuiLayer: failed to create control window");
        return;
    }

    glfwMakeContextCurrent(window_);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        SE_LOG_ERROR("ImGuiLayer: glad initialization failed");
        if (ownsWindow_) {
            glfwDestroyWindow(window_);
            window_     = nullptr;
            ownsWindow_ = false;
        }
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    iniFilePath_   = "assets/imgui_" + appName_ + ".ini";
    io.IniFilename = iniFilePath_.c_str();

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window_, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    initialized_ = true;
    SE_LOG_INFO("ImGuiLayer attached using dedicated OpenGL control window");
}

void ImGuiLayer::OnDetach() {
    if (!initialized_) return;

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    initialized_ = false;
    frameActive_ = false;

    if (ownsWindow_ && window_) {
        glfwDestroyWindow(window_);
        window_     = nullptr;
        ownsWindow_ = false;
    }

    SE_LOG_INFO("ImGuiLayer detached");
}

void ImGuiLayer::OnUpdate(float ts) {
    // ImGui doesn't need per-frame update logic here
}

void ImGuiLayer::OnRender() {
    // Rendering is handled by Begin/End
}

void ImGuiLayer::Begin() {
    frameActive_ = false;

    if (!initialized_ || !window_) return;
    if (glfwWindowShouldClose(window_)) return;

    glfwMakeContextCurrent(window_);
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    frameActive_ = true;
}

void ImGuiLayer::End() {
    if (!frameActive_) return;

    ImGui::Render();

    int display_w = 0;
    int display_h = 0;
    glfwGetFramebufferSize(window_, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.08f, 0.08f, 0.10f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window_);
    glfwMakeContextCurrent(nullptr);

    frameActive_ = false;
}

bool ImGuiLayer::IsFrameActive() const {
    return frameActive_;
}

}  // namespace se
