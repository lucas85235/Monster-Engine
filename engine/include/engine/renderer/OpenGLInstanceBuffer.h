#pragma once

#include "engine/renderer/IInstanceBuffer.h"

namespace se {

/**
 * OpenGL implementation of IInstanceBuffer.
 * Uses a VBO with dynamic/static/stream usage hints.
 */
class OpenGLInstanceBuffer : public IInstanceBuffer {
   public:
    OpenGLInstanceBuffer(uint32_t stride, uint32_t maxInstances, InstanceBufferUsage usage);
    ~OpenGLInstanceBuffer() override;

    OpenGLInstanceBuffer(const OpenGLInstanceBuffer&)            = delete;
    OpenGLInstanceBuffer& operator=(const OpenGLInstanceBuffer&) = delete;

    void Bind() const override;
    void Unbind() const override;
    void SetData(const void* data, uint32_t size, uint32_t instanceCount) override;
    void SetSubData(const void* data, uint32_t offset, uint32_t size) override;

    uint32_t GetInstanceCount() const override { return instanceCount_; }
    uint32_t GetStride() const override { return stride_; }
    uint32_t GetHandle() const override { return rendererId_; }

   private:
    uint32_t            rendererId_    = 0;
    uint32_t            stride_        = 0;
    uint32_t            maxInstances_  = 0;
    uint32_t            instanceCount_ = 0;
    InstanceBufferUsage usage_;
};

}  // namespace se
