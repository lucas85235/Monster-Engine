#pragma once

#include <memory>
#include <string>

#include "engine/rhi/rhi_types.h"

namespace se {

class Texture {
   public:
    Texture(const std::string& path);
    Texture(uint32_t width, uint32_t height, RHI::TextureFormat format = RHI::TextureFormat::RGBA8, const void* data = nullptr);
    ~Texture();

    void Bind(uint32_t slot = 0) const;

    uint32_t GetWidth() const {
        return width_;
    }
    uint32_t GetHeight() const {
        return height_;
    }
    RHI::TextureHandle GetHandle() const {
        return handle_;
    }

    static std::shared_ptr<Texture> Create(const std::string& path);
    static std::shared_ptr<Texture> Create(uint32_t width, uint32_t height, RHI::TextureFormat format = RHI::TextureFormat::RGBA8);
    static std::shared_ptr<Texture> CreateWhiteTexture();

   private:
    uint32_t           width_  = 0;
    uint32_t           height_ = 0;
    std::string        path_;
    RHI::TextureHandle handle_ = {0};
};

}  // namespace se
