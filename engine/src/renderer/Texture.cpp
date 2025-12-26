#include "engine/renderer/Texture.h"

#include <stb_image.h>

#include <stdexcept>

#include "engine/core/Application.h"
#include "engine/core/Log.h"
#include "engine/renderer/GraphicsContext.h"

namespace se {

// Helper to get device
static RHI::IDevice* GetDevice() {
    auto& app     = Application::Get();
    auto& window  = app.GetWindow();
    auto* context = window.GetContext();
    if (context) { return context->GetDevice(); }
    return nullptr;
}

Texture::Texture(const std::string& path) : path_(path) {
    int w, h, channels;
    stbi_set_flip_vertically_on_load(1);
    unsigned char* data = stbi_load(path.c_str(), &w, &h, &channels, 4);  // Force 4 channels (RGBA)

    if (!data) {
        SE_LOG_ERROR("Failed to load texture: {}", path);
        return;
    }

    width_  = static_cast<uint32_t>(w);
    height_ = static_cast<uint32_t>(h);

    RHI::TextureDescriptor desc{};
    desc.type            = RHI::TextureType::Texture2D;
    desc.format          = RHI::TextureFormat::RGBA8;
    desc.width           = width_;
    desc.height          = height_;
    desc.mipLevels       = 1;     // Generate mipmaps? RHI has GenerateMipmaps option
    desc.generateMipmaps = true;  // Let RHI generate mipmaps if supported
    desc.data            = data;

    auto* device = GetDevice();
    if (device) {
        handle_ = device->CreateTexture(desc);
    } else {
        SE_LOG_ERROR("GetDevice() returned null in Texture constructor");
    }

    stbi_image_free(data);
}

Texture::Texture(uint32_t width, uint32_t height, RHI::TextureFormat format) : width_(width), height_(height) {
    RHI::TextureDescriptor desc{};
    desc.type            = RHI::TextureType::Texture2D;
    desc.format          = format;
    desc.width           = width_;
    desc.height          = height_;
    desc.mipLevels       = 1;
    desc.generateMipmaps = false;
    desc.data            = nullptr;  // Empty texture

    auto* device = GetDevice();
    if (device) { handle_ = device->CreateTexture(desc); }
}

Texture::~Texture() {
    auto* device = GetDevice();
    if (device && RHI::IsValid(handle_)) { device->DestroyTexture(handle_); }
}

void Texture::Bind(uint32_t slot) const {
    auto* device = GetDevice();
    if (device) { device->BindTexture(slot, handle_); }
}

std::shared_ptr<Texture> Texture::Create(const std::string& path) {
    return std::make_shared<Texture>(path);
}

std::shared_ptr<Texture> Texture::Create(uint32_t width, uint32_t height, RHI::TextureFormat format) {
    return std::make_shared<Texture>(width, height, format);
}

}  // namespace se
