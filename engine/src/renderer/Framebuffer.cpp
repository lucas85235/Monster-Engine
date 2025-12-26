#include "engine/renderer/Framebuffer.h"

#include "engine/core/Application.h"
#include "engine/core/Log.h"
#include "engine/renderer/GraphicsContext.h"

namespace se {

static RHI::IDevice* GetDevice() {
    auto& app     = Application::Get();
    auto& window  = app.GetWindow();
    auto* context = window.GetContext();
    return context ? context->GetDevice() : nullptr;
}

Framebuffer::Framebuffer(const FramebufferSpecification& spec) : spec_(spec) {
    auto* device = GetDevice();
    if (!device) {
        SE_LOG_ERROR("Failed to get device for Framebuffer creation");
        return;
    }

    RHI::FramebufferDescriptor desc;
    desc.width    = spec.width;
    desc.height   = spec.height;
    desc.hasDepth = spec.hasDepth;
    // desc.colorAttachments... we can configure default color attachment if needed
    // For now, simple creation.

    handle_ = device->CreateFramebuffer(desc);
    if (!RHI::IsValid(handle_)) { SE_LOG_ERROR("Failed to create RHI framebuffer"); }
}

Framebuffer::~Framebuffer() {
    auto* device = GetDevice();
    if (device && RHI::IsValid(handle_)) { device->DestroyFramebuffer(handle_); }
}

void Framebuffer::Bind() const {
    auto* device = GetDevice();
    if (device) { device->BindFramebuffer(handle_); }
}

void Framebuffer::Unbind() const {
    auto* device = GetDevice();
    if (device) {
        device->BindFramebuffer({0});  // Bind default
    }
}

void Framebuffer::Resize(uint32_t width, uint32_t height) {
    if (width == 0 || height == 0 || (width == spec_.width && height == spec_.height)) return;

    spec_.width  = width;
    spec_.height = height;

    auto* device = GetDevice();
    if (device) { device->ResizeFramebuffer(handle_, width, height); }
}

void Framebuffer::AttachTexture(RHI::FramebufferAttachment attachment, const std::shared_ptr<Texture>& texture) {
    auto* device = GetDevice();
    if (device && texture) { device->AttachTexture(handle_, attachment, texture->GetHandle()); }
}

std::shared_ptr<Framebuffer> Framebuffer::Create(const FramebufferSpecification& spec) {
    return std::make_shared<Framebuffer>(spec);
}

}  // namespace se
