#pragma once

#include "engine/Layer.h"
#include "engine/renderer/FrameBuffer.h"

struct GLFWwindow;

namespace se {

class EngineConsole
{
public:
    // Estrutura simples para armazenar o log
    std::vector<std::string> Items;
    bool ScrollToBottom = true; // Flag para rolar automaticamente

    void AddLog(const char* fmt, ...) // Função para adicionar logs
    {
        char buf[1024];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, IM_ARRAYSIZE(buf), fmt, args);
        va_end(args);
        Items.push_back(buf);
        ScrollToBottom = true;
    }

    void Clear()
    {
        Items.clear();
    }

    void Draw()
    {
        // A função ImGui::Begin() usa o nome definido no DockBuilder
        if (!ImGui::Begin("Console"))
        {
            ImGui::End();
            return;
        }

        // 1. Botão "Clear"
        if (ImGui::Button("Clear"))
            Clear();
        ImGui::SameLine();
        ImGui::Text("Sample log message"); // Texto ao lado

        ImGui::Separator();

        // 2. Área de exibição do Log
        // ImGui::BeginChild é importante para ter uma área de rolagem separada
        ImGui::BeginChild("ScrollingRegion", ImVec2(0, 0), false, ImGuiWindowFlags_HorizontalScrollbar);

        // Exibe todas as mensagens
        for (int i = 0; i < Items.size(); i++)
        {
            // Você pode adicionar cores ou formatação aqui dependendo do tipo de log (info, warning, error)
            ImGui::TextUnformatted(Items[i].c_str());
        }

        // 3. Auto-Scroll
        if (ScrollToBottom || (ImGui::IsWindowFocused() && ImGui::GetScrollY() >= ImGui::GetScrollMaxY()))
            ImGui::SetScrollHereY(1.0f); // Rola para o final
        ScrollToBottom = false;

        ImGui::EndChild();
        ImGui::End();
    }
};

class ImGuiLayer : public Layer {
   public:
    ImGuiLayer();
    ~ImGuiLayer() override;

    void OnAttach() override;
    void OnDetach() override;
    void OnUpdate(float ts) override;
    void OnRender() override;
    void OnEvent(Event& event) override;

    void Begin();  // Start new ImGui frame
    void End();    // Render ImGui draw data

    void RenderEditor(uint32_t textureID);
    void RenderViewport(GLuint texture_id);
    void RenderMainWindow(bool& first_time);
    void FrameBufferBind();
    void FrameBufferUnbind();

    void SetWindow(GLFWwindow* window);

   private:
    GLFWwindow* window_ = nullptr;
    EngineConsole myConsole;
    Ref<FrameBuffer> s_FrameBuffer;
};



}  // namespace se