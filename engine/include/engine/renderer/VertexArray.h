#pragma once

#include <memory>
#include <vector>

#include "engine/renderer/Buffer.h"
#include "engine/rhi/rhi_types.h"

namespace se {

class IInstanceBuffer;

class VertexArray {
   public:
    VertexArray();
    ~VertexArray();

    void Bind() const;
    void Unbind() const;

    void AddVertexBuffer(const std::shared_ptr<VertexBuffer>& vertexBuffer);
    void AddInstanceBuffer(const std::shared_ptr<IInstanceBuffer>& instanceBuffer, const BufferLayout& layout);
    void SetIndexBuffer(const std::shared_ptr<IndexBuffer>& indexBuffer);

    const std::vector<std::shared_ptr<VertexBuffer>>& GetVertexBuffers() const {
        return vertexBuffers_;
    }
    const std::shared_ptr<IInstanceBuffer>& GetInstanceBuffer() const {
        return instanceBuffer_;
    }
    const std::shared_ptr<IndexBuffer>& GetIndexBuffer() const {
        return indexBuffer_;
    }

    const BufferLayout& GetInstanceBufferLayout() const {
        return instanceBufferLayout_;
    }

   private:
   private:
    RHI::VertexArrayHandle                     handle_            = {0};
    uint32_t                                   vertexBufferIndex_ = 0;  // Tracks attrib index
    std::vector<std::shared_ptr<VertexBuffer>> vertexBuffers_;
    std::shared_ptr<IInstanceBuffer>           instanceBuffer_;
    BufferLayout                               instanceBufferLayout_;
    std::shared_ptr<IndexBuffer>               indexBuffer_;
};

}  // namespace se
