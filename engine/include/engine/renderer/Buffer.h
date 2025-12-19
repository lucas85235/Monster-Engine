#pragma once

#include <cstdint>
#include <vector>

#include "engine/rhi/rhi_types.h"

namespace se {

// Vertex Buffer Layout
enum class ShaderDataType { None = 0, Float, Float2, Float3, Float4, Mat3, Mat4, Int, Int2, Int3, Int4, Bool };

struct BufferElement {
    std::string    Name;
    ShaderDataType Type;
    uint32_t       Size;
    uint32_t       Offset;
    bool           Normalized;
    uint32_t       InstanceDivisor = 0;  // 0 = per-vertex, 1+ = per-N-instances

    BufferElement() = default;
    BufferElement(ShaderDataType type, const std::string& name, bool normalized = false);
    BufferElement(ShaderDataType type, const std::string& name, uint32_t instanceDivisor);

    uint32_t GetComponentCount() const;
};

class BufferLayout {
   public:
    BufferLayout() = default;
    BufferLayout(const std::initializer_list<BufferElement>& elements);

    uint32_t GetStride() const {
        return stride_;
    }
    const std::vector<BufferElement>& GetElements() const {
        return elements_;
    }

    std::vector<BufferElement>::iterator begin() {
        return elements_.begin();
    }
    std::vector<BufferElement>::iterator end() {
        return elements_.end();
    }
    std::vector<BufferElement>::const_iterator begin() const {
        return elements_.begin();
    }
    std::vector<BufferElement>::const_iterator end() const {
        return elements_.end();
    }

   private:
    void CalculateOffsetsAndStride();

   private:
    std::vector<BufferElement> elements_;
    uint32_t                   stride_ = 0;
};

// Vertex Buffer
class VertexBuffer {
   public:
    VertexBuffer(const void* vertices, uint32_t size);
    VertexBuffer(uint32_t size);  // Dynamic buffer
    ~VertexBuffer();

    void Bind() const;
    void Unbind() const;

    void SetData(const void* data, uint32_t size);

    const BufferLayout& GetLayout() const {
        return layout_;
    }
    void SetLayout(const BufferLayout& layout) {
        layout_ = layout;
    }

    RHI::BufferHandle GetHandle() const {
        return handle_;
    }

   private:
    RHI::BufferHandle handle_ = {0};
    BufferLayout      layout_;
};

// Index Buffer
class IndexBuffer {
   public:
    IndexBuffer(const uint32_t* indices, uint32_t count);
    ~IndexBuffer();

    void Bind() const;
    void Unbind() const;

    uint32_t GetCount() const {
        return count_;
    }

    RHI::BufferHandle GetHandle() const {
        return handle_;
    }

   private:
    RHI::BufferHandle handle_ = {0};
    uint32_t          count_;
};

}  // namespace se