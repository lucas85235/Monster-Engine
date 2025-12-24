#include "editor/EditorFramebuffer.h"

#include "engine/Log.h"

namespace mst {

EditorFramebuffer::EditorFramebuffer(uint32_t width, uint32_t height)
    : width_(width), height_(height) {
    Create();
}

EditorFramebuffer::~EditorFramebuffer() {
    Destroy();
}

void EditorFramebuffer::Create() {
    if (width_ == 0 || height_ == 0) {
        width_ = 1;
        height_ = 1;
    }

    // Create framebuffer
    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);

    // Create color texture
    glGenTextures(1, &colorAttachment_);
    glBindTexture(GL_TEXTURE_2D, colorAttachment_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorAttachment_, 0);

    // Create depth/stencil renderbuffer
    glGenRenderbuffers(1, &depthAttachment_);
    glBindRenderbuffer(GL_RENDERBUFFER, depthAttachment_);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width_, height_);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, depthAttachment_);

    // Check completeness
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        SE_LOG_ERROR("EditorFramebuffer: Framebuffer is not complete!");
    } else {
        SE_LOG_INFO("EditorFramebuffer: Created {}x{} framebuffer", width_, height_);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void EditorFramebuffer::Destroy() {
    if (fbo_ != 0) {
        glDeleteFramebuffers(1, &fbo_);
        fbo_ = 0;
    }
    if (colorAttachment_ != 0) {
        glDeleteTextures(1, &colorAttachment_);
        colorAttachment_ = 0;
    }
    if (depthAttachment_ != 0) {
        glDeleteRenderbuffers(1, &depthAttachment_);
        depthAttachment_ = 0;
    }
}

void EditorFramebuffer::Bind() {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, width_, height_);
    glEnable(GL_DEPTH_TEST);
}

void EditorFramebuffer::Unbind() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void EditorFramebuffer::Resize(uint32_t width, uint32_t height) {
    if (width == width_ && height == height_) return;
    if (width == 0 || height == 0) return;

    width_ = width;
    height_ = height;

    Destroy();
    Create();
}

}  // namespace mst
