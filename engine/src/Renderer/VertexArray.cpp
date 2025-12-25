#include "engine/renderer/VertexArray.h"

#include <glad/glad.h>

#include "engine/Log.h"
#include "engine/renderer/IInstanceBuffer.h"

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

void VertexArray::AddInstanceBuffer(const std::shared_ptr<IInstanceBuffer>& instanceBuffer,
                                    const BufferLayout&                     layout) {
    glBindVertexArray(rendererId_);
    instanceBuffer->Bind();

    for (const auto& element : layout) {
        // Mat4 requires 4 separate vec4 attribute slots
        if (element.Type == ShaderDataType::Mat4) {
            for (int i = 0; i < 4; i++) {
                glEnableVertexAttribArray(vertexBufferIndex_);
                glVertexAttribPointer(
                    vertexBufferIndex_, 4, GL_FLOAT, GL_FALSE, layout.GetStride(),
                    (const void*)(intptr_t)(element.Offset + sizeof(float) * 4 * i));
                glVertexAttribDivisor(vertexBufferIndex_, element.InstanceDivisor);
                vertexBufferIndex_++;
            }
        }
        // Mat3 requires 3 separate vec3 attribute slots
        else if (element.Type == ShaderDataType::Mat3) {
            for (int i = 0; i < 3; i++) {
                glEnableVertexAttribArray(vertexBufferIndex_);
                glVertexAttribPointer(
                    vertexBufferIndex_, 3, GL_FLOAT, GL_FALSE, layout.GetStride(),
                    (const void*)(intptr_t)(element.Offset + sizeof(float) * 3 * i));
                glVertexAttribDivisor(vertexBufferIndex_, element.InstanceDivisor);
                vertexBufferIndex_++;
            }
        } else {
            glEnableVertexAttribArray(vertexBufferIndex_);
            glVertexAttribPointer(vertexBufferIndex_, element.GetComponentCount(),
                                  ShaderDataTypeToOpenGLBaseType(element.Type),
                                  element.Normalized ? GL_TRUE : GL_FALSE, layout.GetStride(),
                                  (const void*)(intptr_t)element.Offset);
            glVertexAttribDivisor(vertexBufferIndex_, element.InstanceDivisor);
            vertexBufferIndex_++;
        }
    }

    instanceBuffer_ = instanceBuffer;
    SE_LOG_INFO("Added instance buffer to VAO (attrib index now at {})", vertexBufferIndex_);
}

void VertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer) {
    glBindVertexArray(rendererId_);
    indexBuffer->Bind();
    indexBuffer_ = indexBuffer;
}

}  // namespace se
