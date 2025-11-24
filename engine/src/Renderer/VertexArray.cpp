#include "engine/renderer/VertexArray.h"
#include <glad/glad.h>

namespace se {

static GLenum ShaderDataTypeToOpenGLBaseType(ShaderDataType type) {
    switch (type) {
        case ShaderDataType::Float:
        case ShaderDataType::Float2:
        case ShaderDataType::Float3:
        case ShaderDataType::Float4:
        case ShaderDataType::Mat3:
        case ShaderDataType::Mat4:
            return GL_FLOAT;
        case ShaderDataType::Int:
        case ShaderDataType::Int2:
        case ShaderDataType::Int3:
        case ShaderDataType::Int4:
            return GL_INT;
        case ShaderDataType::Bool:
            return GL_BOOL;
        default:
            return 0;
    }
}

VertexArray::VertexArray() {
    glGenVertexArrays(1, &rendererId_);
}

VertexArray::~VertexArray() {
    glDeleteVertexArrays(1, &rendererId_);
}

void VertexArray::Bind() const {
    glBindVertexArray(rendererId_);
}

void VertexArray::Unbind() const {
    glBindVertexArray(0);
}

void VertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer) {
    if (vertexBuffer->GetLayout().GetElements().size() == 0) {
        throw std::runtime_error("Vertex Buffer has no layout!");
    }

    glBindVertexArray(rendererId_);
    vertexBuffer->Bind();

    const auto& layout = vertexBuffer->GetLayout();
    for (const auto& element : layout) {
        glEnableVertexAttribArray(vertexBufferIndex_);
        glVertexAttribPointer(vertexBufferIndex_, element.GetComponentCount(),
                              ShaderDataTypeToOpenGLBaseType(element.Type),
                              element.Normalized ? GL_TRUE : GL_FALSE, layout.GetStride(),
                              (const void*)(intptr_t)element.Offset);
        vertexBufferIndex_++;
    }

    vertexBuffers_.push_back(vertexBuffer);
}

void VertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer) {
    glBindVertexArray(rendererId_);
    indexBuffer->Bind();
    indexBuffer_ = indexBuffer;
}

} // namespace se