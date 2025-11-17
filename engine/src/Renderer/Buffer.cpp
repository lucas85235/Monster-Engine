#include "engine/renderer/Buffer.h"

#include <glad/glad.h>

#include <stdexcept>

namespace se {

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
    : Name(name), Type(type), Size(ShaderDataTypeSize(type)), Offset(0), Normalized(normalized) {}

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

BufferLayout::BufferLayout(const std::initializer_list<BufferElement>& elements) : elements_(elements) {
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
    glGenBuffers(1, &rendererId_);
    glBindBuffer(GL_ARRAY_BUFFER, rendererId_);
    glBufferData(GL_ARRAY_BUFFER, size, vertices, GL_STATIC_DRAW);
}

VertexBuffer::VertexBuffer(uint32_t size) {
    glGenBuffers(1, &rendererId_);
    glBindBuffer(GL_ARRAY_BUFFER, rendererId_);
    glBufferData(GL_ARRAY_BUFFER, size, nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

VertexBuffer::~VertexBuffer() {
    glDeleteBuffers(1, &rendererId_);
}

void VertexBuffer::Bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, rendererId_);
}

void VertexBuffer::Unbind() const {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexBuffer::SetData(const void* data, uint32_t size) {
    glBindBuffer(GL_ARRAY_BUFFER, rendererId_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
}

// ========== IndexBuffer ==========

IndexBuffer::IndexBuffer(const uint32_t* indices, uint32_t count) : count_(count) {
    glGenBuffers(1, &rendererId_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rendererId_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(uint32_t), indices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

IndexBuffer::~IndexBuffer() {
    glDeleteBuffers(1, &rendererId_);
}

void IndexBuffer::Bind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, rendererId_);
}

void IndexBuffer::Unbind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

}  // namespace se