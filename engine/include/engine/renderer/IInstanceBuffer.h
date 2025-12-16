#pragma once

#include <cstdint>
#include <memory>

namespace se {

enum class InstanceBufferUsage {
    Static,   // Data set once, used many times
    Dynamic,  // Data updated frequently (e.g., every frame)
    Stream    // Data set once, used once or twice
};

/**
 * IInstanceBuffer - Abstract interface for storing per-instance data.
 * Instance data (transforms, colors, etc.) is uploaded to GPU memory
 * and used with instanced draw calls.
 */
class IInstanceBuffer {
   public:
    virtual ~IInstanceBuffer() = default;

    virtual void Bind() const   = 0;
    virtual void Unbind() const = 0;

    virtual void SetData(const void* data, uint32_t size, uint32_t instanceCount)  = 0;
    virtual void SetSubData(const void* data, uint32_t offset, uint32_t size)      = 0;

    virtual uint32_t GetInstanceCount() const = 0;
    virtual uint32_t GetStride() const        = 0;
    virtual uint32_t GetHandle() const        = 0;
};

std::unique_ptr<IInstanceBuffer> CreateInstanceBuffer(uint32_t stride, uint32_t maxInstances,
                                                       InstanceBufferUsage usage = InstanceBufferUsage::Dynamic);

}  // namespace se
