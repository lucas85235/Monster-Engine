#include "engine/animation/SkinnedMesh.h"

#include <glad/glad.h>

#include "engine/Log.h"
#include "engine/renderer/TextureMaterial.h"

namespace se {

// Vertex layout for skinned mesh:
// Position (3) + Normal (3) + TexCoord (2) + Tangent (3) + Bitangent (3) + BoneIds (4i) + BoneWeights (4f) = 22 floats + 4 ints
struct SkinnedVertexGPU {
    glm::vec3 Position;
    glm::vec3 Normal;
    glm::vec2 TexCoord;
    glm::vec3 Tangent;
    glm::vec3 Bitangent;
    int BoneIds[4];
    float BoneWeights[4];
};

void SkinnedMesh::Create(const SkinnedSubMeshData& data) {
    name_ = data.Name;
    
    if (data.Vertices.empty() || data.Indices.empty()) {
        SE_LOG_WARN("SkinnedMesh::Create: Empty data for '{}'", name_);
        return;
    }
    
    // Convert to GPU format
    std::vector<SkinnedVertexGPU> gpuVertices;
    gpuVertices.reserve(data.Vertices.size());
    
    for (const auto& v : data.Vertices) {
        SkinnedVertexGPU gpuV;
        gpuV.Position = v.Position;
        gpuV.Normal = v.Normal;
        gpuV.TexCoord = v.TexCoord;
        gpuV.Tangent = v.Tangent;
        gpuV.Bitangent = v.Bitangent;
        for (int i = 0; i < 4; ++i) {
            gpuV.BoneIds[i] = v.BoneIds[i];
            gpuV.BoneWeights[i] = v.BoneWeights[i];
        }
        gpuVertices.push_back(gpuV);
    }
    
    // Create VAO
    glGenVertexArrays(1, &vaoId_);
    glGenBuffers(1, &vboId_);
    glGenBuffers(1, &eboId_);
    
    glBindVertexArray(vaoId_);
    
    // VBO
    glBindBuffer(GL_ARRAY_BUFFER, vboId_);
    glBufferData(GL_ARRAY_BUFFER, gpuVertices.size() * sizeof(SkinnedVertexGPU), gpuVertices.data(), GL_STATIC_DRAW);
    
    // EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, eboId_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, data.Indices.size() * sizeof(uint32_t), data.Indices.data(), GL_STATIC_DRAW);
    
    // Vertex attributes
    size_t stride = sizeof(SkinnedVertexGPU);
    
    // Position (location 0)
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SkinnedVertexGPU, Position));
    
    // Normal (location 1)
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SkinnedVertexGPU, Normal));
    
    // TexCoord (location 2)
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SkinnedVertexGPU, TexCoord));
    
    // Tangent (location 3)
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SkinnedVertexGPU, Tangent));
    
    // Bitangent (location 4)
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SkinnedVertexGPU, Bitangent));
    
    // BoneIds (location 5) - integer attribute
    glEnableVertexAttribArray(5);
    glVertexAttribIPointer(5, 4, GL_INT, stride, (void*)offsetof(SkinnedVertexGPU, BoneIds));
    
    // BoneWeights (location 6)
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(SkinnedVertexGPU, BoneWeights));
    
    glBindVertexArray(0);
    
    indexCount_ = static_cast<uint32_t>(data.Indices.size());
    
    SE_LOG_INFO("SkinnedMesh::Create: '{}' - {} vertices, {} indices", name_, gpuVertices.size(), indexCount_);
}

void SkinnedMesh::Draw() const {
    if (vaoId_ == 0 || indexCount_ == 0) return;
    
    glBindVertexArray(vaoId_);
    glDrawElements(GL_TRIANGLES, indexCount_, GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

SkinnedMesh::~SkinnedMesh() {
    if (vaoId_) glDeleteVertexArrays(1, &vaoId_);
    if (vboId_) glDeleteBuffers(1, &vboId_);
    if (eboId_) glDeleteBuffers(1, &eboId_);
}

SkinnedMesh::SkinnedMesh(SkinnedMesh&& other) noexcept
    : name_(std::move(other.name_)),
      vaoId_(other.vaoId_),
      vboId_(other.vboId_),
      eboId_(other.eboId_),
      material_(std::move(other.material_)),
      indexCount_(other.indexCount_) {
    other.vaoId_ = 0;
    other.vboId_ = 0;
    other.eboId_ = 0;
    other.indexCount_ = 0;
}

SkinnedMesh& SkinnedMesh::operator=(SkinnedMesh&& other) noexcept {
    if (this != &other) {
        if (vaoId_) glDeleteVertexArrays(1, &vaoId_);
        if (vboId_) glDeleteBuffers(1, &vboId_);
        if (eboId_) glDeleteBuffers(1, &eboId_);
        
        name_ = std::move(other.name_);
        vaoId_ = other.vaoId_;
        vboId_ = other.vboId_;
        eboId_ = other.eboId_;
        material_ = std::move(other.material_);
        indexCount_ = other.indexCount_;
        
        other.vaoId_ = 0;
        other.vboId_ = 0;
        other.eboId_ = 0;
        other.indexCount_ = 0;
    }
    return *this;
}

}  // namespace se
