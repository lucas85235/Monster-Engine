#pragma once

#include <memory>
#include <glm.hpp>

namespace se {

// Forward declarations
class VertexArray;
class Shader;

/**
 * IRenderBackend - Abstract interface for rendering backends.
 * Allows swapping between OpenGL, Vulkan, DirectX, etc.
 * Follows the Strategy pattern for renderer implementation.
 */
class IRenderBackend {
public:
    virtual ~IRenderBackend() = default;

    // Lifecycle
    virtual void Init() = 0;
    virtual void Shutdown() = 0;

    // Viewport and clear
    virtual void SetViewport(int x, int y, int width, int height) = 0;
    virtual void SetClearColor(float r, float g, float b, float a) = 0;
    virtual void Clear() = 0;

    // Drawing
    virtual void DrawIndexed(VertexArray* va) = 0;
    virtual void DrawIndexedInstanced(VertexArray* va, uint32_t instanceCount) = 0;
    virtual void DrawArraysInstanced(VertexArray* va, uint32_t vertexCount, uint32_t instanceCount) = 0;

    // State management
    virtual void EnableDepthTest(bool enable) = 0;
    virtual void EnableBlending(bool enable) = 0;
    virtual void SetBlendFunc(int srcFactor, int dstFactor) = 0;
    virtual void EnableCullFace(bool enable) = 0;
    virtual void SetCullFaceMode(int mode) = 0;

    // Framebuffer operations
    virtual unsigned int CreateFramebuffer() = 0;
    virtual void DeleteFramebuffer(unsigned int fbo) = 0;
    virtual void BindFramebuffer(unsigned int fbo) = 0;
    virtual void UnbindFramebuffer() = 0;

    // Texture operations
    virtual unsigned int CreateDepthTexture(int width, int height) = 0;
    virtual void DeleteTexture(unsigned int texture) = 0;
    virtual void BindTexture(unsigned int slot, unsigned int texture) = 0;
    virtual void AttachDepthTexture(unsigned int fbo, unsigned int texture) = 0;

    // Utility
    virtual void SaveViewport(int* viewport) = 0;
    virtual void RestoreViewport(const int* viewport) = 0;
};

/**
 * Factory function to create the appropriate backend.
 * Currently only OpenGL is supported.
 */
std::unique_ptr<IRenderBackend> CreateRenderBackend();

}  // namespace se
