#pragma once

#include "engine/renderer/Buffer.h"
#include <memory>
#include <vector>

namespace se {

class VertexArray {
  public:
    VertexArray();
    ~VertexArray();

    void Bind() const;
    void Unbind() const;

    void AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer);
    void SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer);

    const std::vector<std::shared_ptr<VertexBuffer>>& GetVertexBuffers() const {
        return vertexBuffers_;
    }
    const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const {
        return indexBuffer_;
    }

  private:
    uint32_t rendererId_;
    uint32_t vertexBufferIndex_ = 0;
    std::vector<std::shared_ptr<VertexBuffer>> vertexBuffers_;
    std::shared_ptr<IndexBuffer> indexBuffer_;
};

} // namespace se