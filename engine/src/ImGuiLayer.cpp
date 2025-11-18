#include "engine/ImGuiLayer.h"
#include "engine/renderer/FrameBuffer.h"

#include <GLFW/glfw3.h>
#include <imgui.h>
#include "imgui_internal.h"

#include "engine/Log.h"
#include "examples/imgui_impl_glfw.h"
#include "examples/imgui_impl_opengl3.h"
#include "engine/renderer/RenderCommand.h"

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
    static bool can = true;
    RenderMainWindow(can);
    // RenderViewport(my_texture_id, ViewportWidth, ViewportHeight);
    // myConsole.Draw();
}

void ImGuiLayer::OnEvent(Event& event) {
    // You can block events here if ImGui wants to capture them
    // For now, we let them pass through
}

void ImGuiLayer::RenderEditor(uint32_t textureID) {
    static bool can = true;
    RenderMainWindow(can);
    RenderViewport(textureID);
    myConsole.Draw();
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

void ImGuiLayer::RenderViewport(GLuint texture_id)
{
    // Janela "Viewport" (definida no DockBuilder)
    if (ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse))
    {
        // 1. Obter o tamanho disponível da região, para usar no glViewport e redimensionamento do FBO
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();

        // 2. Exibir a Textura da sua engine

        // ImGui::Image espera um ponteiro void* para a ID da textura (casting seguro)
        // Note o uso de UVs invertidos (0, 1) a (1, 0).
        // Isso é necessário porque o OpenGL/Vulkan/DirectX geralmente consideram o Y=0 como a parte inferior,
        // mas o ImGui renderiza texturas como a maioria dos GUIs (Y=0 no topo).
        ImGui::Image(
                (void*)(intptr_t)texture_id, // ID da Textura
                viewportSize,                // Tamanho para preencher a área
                ImVec2(0, 1),                // UV Min (Invertido em Y)
                ImVec2(1, 0)                 // UV Max (Invertido em Y)
        );

        // 3. (Opcional) Capturar eventos de mouse na Viewport
        // Se você quiser que o mouse na Viewport controle sua câmera (orbit, zoom, pan),
        // você precisa garantir que o ImGui reconheça que a Viewport tem foco:
        if (ImGui::IsWindowHovered())
        {
            // Trate a entrada do usuário aqui (por exemplo, mover a câmera)
        }
    }
    ImGui::End();
}

void ImGuiLayer::RenderMainWindow(bool& first_time)
{
    // Janela principal que cobre a tela
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags =
            ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking |
            ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
            ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);

    // Passamos nullptr para o 'p_open' para torná-la sempre aberta
    ImGui::Begin("DockSpace Demo", nullptr, window_flags);

    ImGui::PopStyleVar(2);

    ImGuiID dockspace_id = ImGui::GetID("MyEngineDockSpace");
    ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_PassthruCentralNode;

    // Cria o dockspace
    ImGui::DockSpace(dockspace_id, ImVec2(0, 0), dockspace_flags);

    // Configuração inicial do layout (SÓ NA PRIMEIRA VEZ)
    if (first_time)
    {
        first_time = false;

        // Limpa e cria o nó principal
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, dockspace_flags | ImGuiDockNodeFlags_DockSpace);

        ImGuiID dock_main_id = dockspace_id;
        ImGuiID dock_console_id;

        // Split vertical: 20% para baixo (Console), 80% para cima (Viewport)
        ImGui::DockBuilderSplitNode(dock_main_id, ImGuiDir_Down, 0.2f, &dock_console_id, &dock_main_id);

        // Anexa as janelas aos nós
        ImGui::DockBuilderDockWindow("Viewport", dock_main_id);
        ImGui::DockBuilderDockWindow("Console", dock_console_id);

        // Finaliza a construção
        ImGui::DockBuilderFinish(dockspace_id);
    }

    ImGui::End(); // Fim da janela principal
}

void RenderViewport(GLuint texture_id, int width, int height)
{
    // A função ImGui::Begin() usa o nome definido no DockBuilder
    if (ImGui::Begin("Viewport"))
    {
        // 1. Obter o tamanho atual da janela Viewport
        ImVec2 viewportSize = ImGui::GetContentRegionAvail();

        // **Ajustar FBO:** É aqui que você chamaria a função da sua engine
        // para redimensionar o Frame Buffer Object (FBO) para viewportSize.
        // SuaEngine::ResizeFBO(viewportSize.x, viewportSize.y);

        // 2. Renderizar a textura da sua cena 3D/2D
        // Usamos ImGui::Image() para exibir a textura.
        // O `texture_id` é o ID OpenGL/Vulkan/DirectX da sua textura renderizada.

        // As coordenadas UV (0, 1) a (1, 0) são usadas para flipar a imagem verticalmente,
        // pois em OpenGL/Vulkan o Y=0 geralmente está embaixo, enquanto o ImGui renderiza
        // texturas de cima para baixo.
        ImGui::Image(
                (void*)(intptr_t)texture_id, // ID da Textura (Exemplo GL)
                viewportSize,                // Tamanho
                ImVec2(0, 1),                // UV Min (X=0, Y=1 para flipar)
                ImVec2(1, 0)                 // UV Max (X=1, Y=0 para flipar)
        );
    }
    ImGui::End();
}

}  // namespace se