#pragma once

#include "engine/renderer/IRenderBackend.h"

namespace se {

/**
 * OpenGL implementation of IRenderBackend.
 * Wraps all OpenGL-specific calls for rendering operations.
 */
class OpenGLBackend : public IRenderBackend {
public:
    OpenGLBackend() = default;
    ~OpenGLBackend() override = default;

    // Lifecycle
    void Init() override;
    void Shutdown() override;

    // Viewport and clear
    void SetViewport(int x, int y, int width, int height) override;
    void SetClearColor(float r, float g, float b, float a) override;
    void Clear() override;

    // Drawing
    void DrawIndexed(VertexArray* va) override;
    void DrawIndexedInstanced(VertexArray* va, uint32_t instanceCount) override;
    void DrawArraysInstanced(VertexArray* va, uint32_t vertexCount, uint32_t instanceCount) override;

    // State management
    void EnableDepthTest(bool enable) override;
    void EnableBlending(bool enable) override;
    void SetBlendFunc(int srcFactor, int dstFactor) override;
    void EnableCullFace(bool enable) override;
    void SetCullFaceMode(int mode) override;

    // Framebuffer operations
    unsigned int CreateFramebuffer() override;
    void DeleteFramebuffer(unsigned int fbo) override;
    void BindFramebuffer(unsigned int fbo) override;
    void UnbindFramebuffer() override;

    // Texture operations
    unsigned int CreateDepthTexture(int width, int height) override;
    void DeleteTexture(unsigned int texture) override;
    void BindTexture(unsigned int slot, unsigned int texture) override;
    void AttachDepthTexture(unsigned int fbo, unsigned int texture) override;

    // Utility
    void SaveViewport(int* viewport) override;
    void RestoreViewport(const int* viewport) override;

private:
    float clearColor_[4] = {0.0f, 0.0f, 0.0f, 1.0f};
};

}  // namespace se
