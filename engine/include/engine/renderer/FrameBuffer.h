#pragma once

#include <glad/glad.h>
//#include <glm/glm.hpp>
#include <cstdint>

namespace se {

    struct FramebufferSpecification
    {
        uint32_t Width = 0;
        uint32_t Height = 0;
        // Adicione formatos de textura aqui se precisar de flexibilidade
    };

    class FrameBuffer
    {
    public:
        FrameBuffer(const FramebufferSpecification& spec);
        ~FrameBuffer();

        void Bind();
        void Unbind();

        // Cria/recria o FBO e seus anexos com o novo tamanho
        void Resize(uint32_t width, uint32_t height);

        // Retorna a ID da textura para uso no ImGui::Image
        uint32_t GetColorAttachmentID() const { return m_ColorAttachment; }

        const FramebufferSpecification& GetSpecification() const { return m_Specification; }

    private:
        // Limpeza de recursos OpenGL
        void Invalidate();

        // IDs dos objetos OpenGL
        uint32_t m_RendererID = 0;      // FBO ID
        uint32_t m_ColorAttachment = 0; // Textura de Cor ID
        uint32_t m_DepthAttachment = 0; // RBO/Textura de Profundidade ID (depende da implementação)

        FramebufferSpecification m_Specification;
    };

} // namespace se