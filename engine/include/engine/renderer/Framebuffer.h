#pragma once

#include <memory>
#include <vector>

#include "engine/renderer/Texture.h"
#include "engine/rhi/rhi_types.h"

namespace se {

struct FramebufferSpecification {
    uint32_t width  = 0;
    uint32_t height = 0;
    // Simple specification for now, can be expanded for multiple attachments config
    bool hasDepth = true;
};

class Framebuffer {
   public:
    Framebuffer(const FramebufferSpecification& spec);
    ~Framebuffer();

    void Bind() const;
    void Unbind() const;

    void Resize(uint32_t width, uint32_t height);

    // Attach an existing texture (useful for depth map sharing)
    void AttachTexture(RHI::FramebufferAttachment attachment, const std::shared_ptr<Texture>& texture);

    // Getters
    const FramebufferSpecification& GetSpecification() const {
        return spec_;
    }
    RHI::FramebufferHandle GetHandle() const {
        return handle_;
    }

    static std::shared_ptr<Framebuffer> Create(const FramebufferSpecification& spec);

   private:
    FramebufferSpecification spec_;
    RHI::FramebufferHandle   handle_ = {0};
    // Keep references to attached textures to keep them alive??
    // Or assume ownership is external? For safety, maybe keep them?
    // For shadows, we might create texture externally.
};

}  // namespace se
