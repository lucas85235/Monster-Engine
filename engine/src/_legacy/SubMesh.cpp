#include "engine/resources/SubMesh.h"

#include <glad/glad.h>

#include "engine/Log.h"
#include "engine/renderer/Material.h"
#include "engine/renderer/VertexArray.h"

namespace se {

SubMesh::SubMesh(std::shared_ptr<VertexArray> vertexArray,
                 std::shared_ptr<Material> material,
                 const std::string& name)
    : vertexArray_(std::move(vertexArray)),
      material_(std::move(material)),
      name_(name) {
}

void SubMesh::Draw() const {
    if (!vertexArray_) {
        SE_LOG_WARN("SubMesh::Draw: No vertex array bound for submesh '{}'", name_);
        return;
    }

    vertexArray_->Bind();
    
    if (auto indexBuffer = vertexArray_->GetIndexBuffer()) {
        glDrawElements(GL_TRIANGLES, 
                       static_cast<GLsizei>(indexBuffer->GetCount()), 
                       GL_UNSIGNED_INT, 
                       nullptr);
    }
    
    vertexArray_->Unbind();
}

}  // namespace se
