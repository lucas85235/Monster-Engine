#include "engine/renderer/OpenGLInstanceBuffer.h"

#include <glad/glad.h>

#include "engine/Log.h"

namespace se {

static GLenum UsageToGL(InstanceBufferUsage usage) {
    switch (usage) {
        case InstanceBufferUsage::Static:
            return GL_STATIC_DRAW;
        case InstanceBufferUsage::Dynamic:
            return GL_DYNAMIC_DRAW;
        case InstanceBufferUsage::Stream:
            return GL_STREAM_DRAW;
    }
    return GL_DYNAMIC_DRAW;
}

OpenGLInstanceBuffer::OpenGLInstanceBuffer(uint32_t stride, uint32_t maxInstances,
                                           InstanceBufferUsage usage)
    : stride_(stride), maxInstances_(maxInstances), usage_(usage) {
    glGenBuffers(1, &rendererId_);
    if (rendererId_ == 0) {
        SE_LOG_ERROR("Failed to create instance buffer");
        return;
    }

    glBindBuffer(GL_ARRAY_BUFFER, rendererId_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(stride) * maxInstances, nullptr,
                 UsageToGL(usage));

    GLenum error = glGetError();
    if (error != GL_NO_ERROR) { SE_LOG_ERROR("GL error creating instance buffer: 0x{:X}", error); }

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    SE_LOG_INFO("Created instance buffer (stride={}, maxInstances={}, handle={})", stride,
                maxInstances, rendererId_);
}

OpenGLInstanceBuffer::~OpenGLInstanceBuffer() {
    if (rendererId_) {
        glDeleteBuffers(1, &rendererId_);
        SE_LOG_INFO("Destroyed instance buffer (handle={})", rendererId_);
    }
}

void OpenGLInstanceBuffer::Bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, rendererId_);
}

void OpenGLInstanceBuffer::Unbind() const {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLInstanceBuffer::SetData(const void* data, uint32_t size, uint32_t instanceCount) {
    if (instanceCount > maxInstances_) {
        SE_LOG_WARN("Instance count {} exceeds max {}, clamping", instanceCount, maxInstances_);
        instanceCount = maxInstances_;
        size          = stride_ * maxInstances_;
    }

    instanceCount_ = instanceCount;
    glBindBuffer(GL_ARRAY_BUFFER, rendererId_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, size, data);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void OpenGLInstanceBuffer::SetSubData(const void* data, uint32_t offset, uint32_t size) {
    glBindBuffer(GL_ARRAY_BUFFER, rendererId_);
    glBufferSubData(GL_ARRAY_BUFFER, offset, size, data);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

std::unique_ptr<IInstanceBuffer> CreateInstanceBuffer(uint32_t stride, uint32_t maxInstances,
                                                      InstanceBufferUsage usage) {
    return std::make_unique<OpenGLInstanceBuffer>(stride, maxInstances, usage);
}

}  // namespace se
