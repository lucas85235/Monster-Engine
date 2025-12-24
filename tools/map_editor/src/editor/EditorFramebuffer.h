#pragma once

#include <cstdint>
#include <glad/glad.h>

namespace mst {

class EditorFramebuffer {
   public:
    EditorFramebuffer(uint32_t width, uint32_t height);
    ~EditorFramebuffer();

    void Bind();
    void Unbind();

    void Resize(uint32_t width, uint32_t height);

    uint32_t GetColorAttachment() const { return colorAttachment_; }
    uint32_t GetWidth() const { return width_; }
    uint32_t GetHeight() const { return height_; }

   private:
    void Create();
    void Destroy();

    uint32_t fbo_ = 0;
    uint32_t colorAttachment_ = 0;
    uint32_t depthAttachment_ = 0;
    uint32_t width_ = 0;
    uint32_t height_ = 0;
};

}  // namespace mst
