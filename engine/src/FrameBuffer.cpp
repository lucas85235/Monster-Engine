#include "engine/renderer/FrameBuffer.h"

// Para garantir que as IDs não sejam 0
static constexpr uint32_t s_MaxFramebufferSize = 8192;

namespace se {

    FrameBuffer::FrameBuffer(const FramebufferSpecification& spec)
            : m_Specification(spec)
    {
        // Cria o FBO e os anexos.
        Invalidate();
    }

    FrameBuffer::~FrameBuffer()
    {
        // Limpa recursos ao destruir o objeto.
        glDeleteFramebuffers(1, &m_RendererID);
        glDeleteTextures(1, &m_ColorAttachment);
        glDeleteRenderbuffers(1, &m_DepthAttachment);
    }

// Cria/recria o FBO e todos os seus anexos.
    void FrameBuffer::Invalidate()
    {
        // Se já existem recursos, exclua-os primeiro.
        if (m_RendererID)
        {
            glDeleteFramebuffers(1, &m_RendererID);
            glDeleteTextures(1, &m_ColorAttachment);
            glDeleteRenderbuffers(1, &m_DepthAttachment);
        }

        // 1. Cria o FBO
        glGenFramebuffers(1, &m_RendererID);
        glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);

        // 2. Cria a Textura de Cor (Color Attachment)
        glGenTextures(1, &m_ColorAttachment);
        glBindTexture(GL_TEXTURE_2D, m_ColorAttachment);

        // Define o formato da textura (sem dados por enquanto)
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_Specification.Width, m_Specification.Height,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

        // Configuração de filtro e wrap
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_ColorAttachment, 0);

        // 3. Cria o Render Buffer de Profundidade/Stencil
        glGenRenderbuffers(1, &m_DepthAttachment);
        glBindRenderbuffer(GL_RENDERBUFFER, m_DepthAttachment);
        // Configura o armazenamento de profundidade/stencil
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, m_Specification.Width, m_Specification.Height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_DepthAttachment);

        // 4. Checa se o FBO está completo
        // Você pode adicionar tratamento de erro aqui
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            // ... (Log de erro: "Framebuffer is incomplete!")
        }

        // 5. Desvincula o FBO para voltar ao alvo padrão
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void FrameBuffer::Resize(uint32_t width, uint32_t height)
    {
        if (width == 0 || height == 0 || width > s_MaxFramebufferSize || height > s_MaxFramebufferSize)
        {
            // Log de aviso ou erro se o redimensionamento for inválido
            return;
        }

        m_Specification.Width = width;
        m_Specification.Height = height;

        // Invalidate recria os recursos com o novo tamanho
        Invalidate();
    }

    void FrameBuffer::Bind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_RendererID);
        // IMPORTANTE: Sempre configure o viewport do OpenGL para o tamanho do FBO
        glViewport(0, 0, m_Specification.Width, m_Specification.Height);
    }

    void FrameBuffer::Unbind()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

} // namespace se