#include "engine/renderer/Buffer.h"

#include <stdexcept>

#include "engine/core/Application.h"
#include "engine/core/Log.h"
#include "engine/renderer/GraphicsContext.h"

namespace se {

// Helper to access device
static RHI::IDevice* GetDevice() {
    auto& app     = Application::Get();
    auto* context = app.GetWindow().GetContext();
    return context ? context->GetDevice() : nullptr;
}

// ========== BufferElement ==========

static uint32_t ShaderDataTypeSize(ShaderDataType type) {
    switch (type) {
        case ShaderDataType::Float:
            return 4;
        case ShaderDataType::Float2:
            return 4 * 2;
        case ShaderDataType::Float3:
            return 4 * 3;
        case ShaderDataType::Float4:
            return 4 * 4;
        case ShaderDataType::Mat3:
            return 4 * 3 * 3;
        case ShaderDataType::Mat4:
            return 4 * 4 * 4;
        case ShaderDataType::Int:
            return 4;
        case ShaderDataType::Int2:
            return 4 * 2;
        case ShaderDataType::Int3:
            return 4 * 3;
        case ShaderDataType::Int4:
            return 4 * 4;
        case ShaderDataType::Bool:
            return 1;
        default:
            return 0;
    }
}

BufferElement::BufferElement(ShaderDataType type, const std::string& name, bool normalized)
    : Name(name),
      Type(type),
      Size(ShaderDataTypeSize(type)),
      Offset(0),
      Normalized(normalized),
      InstanceDivisor(0) {}

BufferElement::BufferElement(ShaderDataType type, const std::string& name, uint32_t instanceDivisor)
    : Name(name),
      Type(type),
      Size(ShaderDataTypeSize(type)),
      Offset(0),
      Normalized(false),
      InstanceDivisor(instanceDivisor) {}

uint32_t BufferElement::GetComponentCount() const {
    switch (Type) {
        case ShaderDataType::Float:
            return 1;
        case ShaderDataType::Float2:
            return 2;
        case ShaderDataType::Float3:
            return 3;
        case ShaderDataType::Float4:
            return 4;
        case ShaderDataType::Mat3:
            return 3 * 3;
        case ShaderDataType::Mat4:
            return 4 * 4;
        case ShaderDataType::Int:
            return 1;
        case ShaderDataType::Int2:
            return 2;
        case ShaderDataType::Int3:
            return 3;
        case ShaderDataType::Int4:
            return 4;
        case ShaderDataType::Bool:
            return 1;
        default:
            return 0;
    }
}

// ========== BufferLayout ==========

BufferLayout::BufferLayout(const std::initializer_list<BufferElement>& elements)
    : elements_(elements) {
    CalculateOffsetsAndStride();
}

void BufferLayout::CalculateOffsetsAndStride() {
    uint32_t offset = 0;
    stride_         = 0;
    for (auto& element : elements_) {
        element.Offset = offset;
        offset += element.Size;
        stride_ += element.Size;
    }
}

// ========== VertexBuffer ==========

VertexBuffer::VertexBuffer(const void* vertices, uint32_t size) {
    auto* device = GetDevice();
    if (!device) {
        SE_LOG_ERROR("Failed to get RHI device in VertexBuffer constructor");
        return;
    }

    RHI::BufferDescriptor desc{};
    desc.type  = RHI::BufferType::Vertex;
    desc.usage = RHI::BufferUsage::Static;
    desc.size  = size;
    desc.data  = vertices;

    handle_ = device->CreateBuffer(desc);
}

VertexBuffer::VertexBuffer(uint32_t size) {
    auto* device = GetDevice();
    if (!device) {
        SE_LOG_ERROR("Failed to get RHI device in VertexBuffer constructor");
        return;
    }

    RHI::BufferDescriptor desc{};
    desc.type  = RHI::BufferType::Vertex;
    desc.usage = RHI::BufferUsage::Dynamic;
    desc.size  = size;
    desc.data  = nullptr;

    handle_ = device->CreateBuffer(desc);
}

VertexBuffer::~VertexBuffer() {
    auto* device = GetDevice();
    if (device && RHI::IsValid(handle_)) { device->DestroyBuffer(handle_); }
}

void VertexBuffer::Bind() const {
    // RHI binding is handled via VertexArray/Pipeline.
    // Keeping this method for interface compatibility but it may be a no-op.
    // However, if we are in transition, some legacy GL calls might expect binding.
    // But since we removed GL includes, we can't do GL calls here.
    // So this is effectively a no-op implementation.
}

void VertexBuffer::Unbind() const {
    // No-op
}

void VertexBuffer::SetData(const void* data, uint32_t size) {
    auto* device = GetDevice();
    if (device && RHI::IsValid(handle_)) { device->UpdateBuffer(handle_, data, size, 0); }
}

// ========== IndexBuffer ==========

IndexBuffer::IndexBuffer(const uint32_t* indices, uint32_t count) : count_(count) {
    auto* device = GetDevice();
    if (!device) {
        SE_LOG_ERROR("Failed to get RHI device in IndexBuffer constructor");
        return;
    }

    RHI::BufferDescriptor desc{};
    desc.type  = RHI::BufferType::Index;
    desc.usage = RHI::BufferUsage::Static;
    desc.size  = count * sizeof(uint32_t);
    desc.data  = indices;

    handle_ = device->CreateBuffer(desc);
}

IndexBuffer::~IndexBuffer() {
    auto* device = GetDevice();
    if (device && RHI::IsValid(handle_)) { device->DestroyBuffer(handle_); }
}

void IndexBuffer::Bind() const {
    // No-op
}

void IndexBuffer::Unbind() const {
    // No-op
}

}  // namespace se