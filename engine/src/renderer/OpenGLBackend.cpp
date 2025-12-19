#include "engine/renderer/OpenGLBackend.h"

#include <glad/glad.h>

#include "engine/core/Log.h"
#include "engine/renderer/VertexArray.h"

namespace se {

void OpenGLBackend::Init() {
    SE_LOG_INFO("OpenGL Backend initialized");
    
    // Enable depth testing by default
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Enable back-face culling by default
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
}

void OpenGLBackend::Shutdown() {
    SE_LOG_INFO("OpenGL Backend shutdown");
}

void OpenGLBackend::SetViewport(int x, int y, int width, int height) {
    glViewport(x, y, width, height);
}

void OpenGLBackend::SetClearColor(float r, float g, float b, float a) {
    clearColor_[0] = r;
    clearColor_[1] = g;
    clearColor_[2] = b;
    clearColor_[3] = a;
    glClearColor(r, g, b, a);
}

void OpenGLBackend::Clear() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLBackend::DrawIndexed(VertexArray* va) {
    if (!va) return;
    
    va->Bind();
    auto indexBuffer = va->GetIndexBuffer();
    if (indexBuffer) {
        glDrawElements(GL_TRIANGLES, indexBuffer->GetCount(), GL_UNSIGNED_INT, nullptr);
    }
}

void OpenGLBackend::DrawIndexedInstanced(VertexArray* va, uint32_t instanceCount) {
    if (!va || instanceCount == 0) return;

    va->Bind();
    auto indexBuffer = va->GetIndexBuffer();
    if (indexBuffer) {
        glDrawElementsInstanced(GL_TRIANGLES, indexBuffer->GetCount(), GL_UNSIGNED_INT, nullptr, instanceCount);
    }
}

void OpenGLBackend::DrawArraysInstanced(VertexArray* va, uint32_t vertexCount, uint32_t instanceCount) {
    if (!va || instanceCount == 0) return;

    va->Bind();
    glDrawArraysInstanced(GL_TRIANGLES, 0, vertexCount, instanceCount);
}

void OpenGLBackend::EnableDepthTest(bool enable) {
    if (enable)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
}

void OpenGLBackend::EnableBlending(bool enable) {
    if (enable)
        glEnable(GL_BLEND);
    else
        glDisable(GL_BLEND);
}

void OpenGLBackend::SetBlendFunc(int srcFactor, int dstFactor) {
    glBlendFunc(srcFactor, dstFactor);
}

void OpenGLBackend::EnableCullFace(bool enable) {
    if (enable)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
}

void OpenGLBackend::SetCullFaceMode(int mode) {
    glCullFace(mode);
}

unsigned int OpenGLBackend::CreateFramebuffer() {
    unsigned int fbo;
    glGenFramebuffers(1, &fbo);
    return fbo;
}

void OpenGLBackend::DeleteFramebuffer(unsigned int fbo) {
    glDeleteFramebuffers(1, &fbo);
}

void OpenGLBackend::BindFramebuffer(unsigned int fbo) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
}

void OpenGLBackend::UnbindFramebuffer() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

unsigned int OpenGLBackend::CreateDepthTexture(int width, int height) {
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, 
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float borderColor[4] = {1.0f, 1.0f, 1.0f, 1.0f};
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);
    glBindTexture(GL_TEXTURE_2D, 0);
    return texture;
}

void OpenGLBackend::DeleteTexture(unsigned int texture) {
    glDeleteTextures(1, &texture);
}

void OpenGLBackend::BindTexture(unsigned int slot, unsigned int texture) {
    glActiveTexture(GL_TEXTURE0 + slot);
    glBindTexture(GL_TEXTURE_2D, texture);
}

void OpenGLBackend::AttachDepthTexture(unsigned int fbo, unsigned int texture) {
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, texture, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void OpenGLBackend::SaveViewport(int* viewport) {
    glGetIntegerv(GL_VIEWPORT, viewport);
}

void OpenGLBackend::RestoreViewport(const int* viewport) {
    glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
}

// Factory function implementation
std::unique_ptr<IRenderBackend> CreateRenderBackend() {
    return std::make_unique<OpenGLBackend>();
}

}  // namespace se
