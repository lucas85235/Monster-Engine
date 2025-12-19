#include "engine/renderer/InstanceBuffer.h"

#include "engine/core/Application.h"
#include "engine/core/Log.h"
#include "engine/renderer/GraphicsContext.h"

namespace se {

static RHI::IDevice* GetDevice() {
    auto& app     = Application::Get();
    auto* context = app.GetWindow().GetContext();
    return context ? context->GetDevice() : nullptr;
}

InstanceBuffer::InstanceBuffer(uint32_t stride, uint32_t maxInstances, InstanceBufferUsage usage)
    : stride_(stride), maxInstances_(maxInstances), usage_(usage) {
    auto* device = GetDevice();
    if (!device) {
        SE_LOG_ERROR("Failed to get RHI device in InstanceBuffer constructor");
        return;
    }

    RHI::BufferDescriptor desc{};
    desc.type = RHI::BufferType::Vertex;  // Instance buffers are just vertex buffers

    switch (usage) {
        case InstanceBufferUsage::Static:
            desc.usage = RHI::BufferUsage::Static;
            break;
        case InstanceBufferUsage::Dynamic:
            desc.usage = RHI::BufferUsage::Dynamic;
            break;
        case InstanceBufferUsage::Stream:
            desc.usage = RHI::BufferUsage::Stream;
            break;
    }

    desc.size = stride * maxInstances;
    desc.data = nullptr;  // Initial empty

    handle_ = device->CreateBuffer(desc);

    if (RHI::IsValid(handle_)) { SE_LOG_INFO("Created instance buffer (stride={}, maxInstances={}, handle={})", stride, maxInstances, handle_.id); }
}

InstanceBuffer::~InstanceBuffer() {
    auto* device = GetDevice();
    if (device && RHI::IsValid(handle_)) { device->DestroyBuffer(handle_); }
}

void InstanceBuffer::Bind() const {
    // No-op in RHI
}

void InstanceBuffer::Unbind() const {
    // No-op in RHI
}

void InstanceBuffer::SetData(const void* data, uint32_t size, uint32_t instanceCount) {
    if (instanceCount > maxInstances_) {
        SE_LOG_WARN("Instance count {} exceeds max {}, clamping", instanceCount, maxInstances_);
        instanceCount = maxInstances_;
        size          = stride_ * maxInstances_;
    }

    instanceCount_ = instanceCount;
    auto* device   = GetDevice();
    if (device && RHI::IsValid(handle_)) { device->UpdateBuffer(handle_, data, size, 0); }
}

void InstanceBuffer::SetSubData(const void* data, uint32_t offset, uint32_t size) {
    auto* device = GetDevice();
    if (device && RHI::IsValid(handle_)) { device->UpdateBuffer(handle_, data, size, offset); }
}

std::unique_ptr<IInstanceBuffer> CreateInstanceBuffer(uint32_t stride, uint32_t maxInstances, InstanceBufferUsage usage) {
    return std::make_unique<InstanceBuffer>(stride, maxInstances, usage);
}

}  // namespace se
