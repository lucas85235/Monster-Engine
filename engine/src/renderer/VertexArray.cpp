#include "engine/renderer/VertexArray.h"

#include "engine/core/Application.h"
#include "engine/core/Log.h"
#include "engine/renderer/GraphicsContext.h"
#include "engine/renderer/IInstanceBuffer.h"

namespace se {

static RHI::IDevice* GetDevice() {
    auto& app     = Application::Get();
    auto* context = app.GetWindow().GetContext();
    return context ? context->GetDevice() : nullptr;
}

static RHI::VertexAttributeType ConvertShaderType(ShaderDataType type) {
    switch (type) {
        case ShaderDataType::Float:
            return RHI::VertexAttributeType::Float;
        case ShaderDataType::Float2:
            return RHI::VertexAttributeType::Float2;
        case ShaderDataType::Float3:
            return RHI::VertexAttributeType::Float3;
        case ShaderDataType::Float4:
            return RHI::VertexAttributeType::Float4;
        case ShaderDataType::Mat3:
            return RHI::VertexAttributeType::Float3;  // Mat3 handled as 3x Float3 in layout loop usually
        case ShaderDataType::Mat4:
            return RHI::VertexAttributeType::Float4;  // Mat4 handled as 4x Float4
        case ShaderDataType::Int:
            return RHI::VertexAttributeType::Int;
        case ShaderDataType::Int2:
            return RHI::VertexAttributeType::Int2;
        case ShaderDataType::Int3:
            return RHI::VertexAttributeType::Int3;
        case ShaderDataType::Int4:
            return RHI::VertexAttributeType::Int4;
        case ShaderDataType::Bool:
            return RHI::VertexAttributeType::Int;  // Bool usually int in GLSL attributes
        default:
            return RHI::VertexAttributeType::Float;
    }
}

VertexArray::VertexArray() {}

VertexArray::~VertexArray() {
    auto* device = GetDevice();
    if (device && RHI::IsValid(handle_)) { device->DestroyVertexArray(handle_); }
}

void VertexArray::Bind() const {
    auto* device = GetDevice();
    if (!device) return;

    if (!RHI::IsValid(handle_)) {
        // Create the RHI Vertex Array
        RHI::VertexArrayDescriptor desc;
        uint32_t                   locationIndex = 0;

        // Process standard Vertex Buffers
        for (const auto& vb : vertexBuffers_) {
            RHI::VertexBinding binding;
            binding.buffer        = vb->GetHandle();
            binding.divisor       = 0;
            binding.layout.stride = vb->GetLayout().GetStride();

            for (const auto& element : vb->GetLayout()) {
                // Handle Matrices (Mat3/Mat4) which take multiple attribute slots
                uint32_t count = 1;
                if (element.Type == ShaderDataType::Mat3) count = 3;
                if (element.Type == ShaderDataType::Mat4) count = 4;

                for (uint32_t i = 0; i < count; i++) {
                    RHI::VertexAttribute attr;
                    attr.location   = locationIndex;
                    attr.type       = ConvertShaderType(element.Type);
                    attr.normalized = element.Normalized;

                    uint32_t typeSize = 0;  // Size of the base type (e.g. sizeof(vec4))
                                            // Re-calculate offset based on i
                    if (element.Type == ShaderDataType::Mat3)
                        typeSize = 3 * 4;
                    else if (element.Type == ShaderDataType::Mat4)
                        typeSize = 4 * 4;

                    attr.offset = element.Offset + (i * typeSize);

                    binding.layout.attributes.push_back(attr);
                    locationIndex++;
                }
            }
            desc.bindings.push_back(binding);
        }

        // Process Instance Buffer
        if (instanceBuffer_) {
            RHI::VertexBinding binding;
            binding.buffer        = instanceBuffer_->GetHandle();
            binding.divisor       = 1;  // Per instance
            binding.layout.stride = instanceBufferLayout_.GetStride();

            for (const auto& element : instanceBufferLayout_) {
                // Similar logic for matrices
                uint32_t count = 1;
                if (element.Type == ShaderDataType::Mat3) count = 3;
                if (element.Type == ShaderDataType::Mat4) count = 4;

                for (uint32_t i = 0; i < count; i++) {
                    RHI::VertexAttribute attr;
                    attr.location   = locationIndex;
                    attr.type       = ConvertShaderType(element.Type);
                    attr.normalized = element.Normalized;

                    uint32_t typeSize = 0;
                    if (element.Type == ShaderDataType::Mat3)
                        typeSize = 3 * 4;
                    else if (element.Type == ShaderDataType::Mat4)
                        typeSize = 4 * 4;

                    attr.offset = element.Offset + (i * typeSize);

                    binding.layout.attributes.push_back(attr);
                    locationIndex++;
                }
            }
            desc.bindings.push_back(binding);
        }

        if (indexBuffer_) { desc.indexBuffer = indexBuffer_->GetHandle(); }

        // We can cast away constness here because this is lazy initialization of internal cache
        auto* mutableThis    = const_cast<VertexArray*>(this);
        mutableThis->handle_ = device->CreateVertexArray(desc);
    }

    device->BindVertexArray(handle_);
}

void VertexArray::Unbind() const {
    auto* device = GetDevice();
    if (device) { device->BindVertexArray({0}); }
}

void VertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer) {
    vertexBuffers_.push_back(vertexBuffer);

    // Invalidate handle to force recreation
    auto* device = GetDevice();
    if (device && RHI::IsValid(handle_)) {
        device->DestroyVertexArray(handle_);
        handle_ = {0};
    }
}

void VertexArray::AddInstanceBuffer(const std::shared_ptr<IInstanceBuffer>& instanceBuffer, const BufferLayout& layout) {
    instanceBuffer_       = instanceBuffer;
    instanceBufferLayout_ = layout;

    // Invalidate handle
    auto* device = GetDevice();
    if (device && RHI::IsValid(handle_)) {
        device->DestroyVertexArray(handle_);
        handle_ = {0};
    }
}

void VertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer) {
    indexBuffer_ = indexBuffer;

    // Invalidate handle
    auto* device = GetDevice();
    if (device && RHI::IsValid(handle_)) {
        device->DestroyVertexArray(handle_);
        handle_ = {0};
    }
}

}  // namespace se
