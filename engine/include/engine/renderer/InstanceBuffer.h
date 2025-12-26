#pragma once

#include "engine/renderer/IInstanceBuffer.h"
#include "engine/rhi/rhi_types.h"

namespace se {

class InstanceBuffer : public IInstanceBuffer {
   public:
    InstanceBuffer(uint32_t stride, uint32_t maxInstances, InstanceBufferUsage usage);
    ~InstanceBuffer() override;

    void Bind() const override;
    void Unbind() const override;

    void SetData(const void* data, uint32_t size, uint32_t instanceCount) override;
    void SetSubData(const void* data, uint32_t offset, uint32_t size) override;

    uint32_t GetInstanceCount() const override {
        return instanceCount_;
    }
    uint32_t GetStride() const override {
        return stride_;
    }
    RHI::BufferHandle GetHandle() const override {
        return handle_;
    }

   private:
    uint32_t            stride_;
    uint32_t            maxInstances_;
    uint32_t            instanceCount_ = 0;
    InstanceBufferUsage usage_;
    RHI::BufferHandle   handle_ = {0};
};

}  // namespace se
