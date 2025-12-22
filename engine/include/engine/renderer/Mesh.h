#pragma once

#include <glad/glad.h>

#include <vector>

class Mesh {
   public:
    Mesh() = default;
    Mesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices);
    Mesh(const Mesh&)            = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;
    ~Mesh();

    // Render using legacy OpenGL (for backward compatibility)
    void draw() const;

    // Getters for new renderer architecture
    const std::vector<float>& getVertices() const {
        return vertices_;
    }
    const std::vector<unsigned int>& getIndices() const {
        return indices_;
    }

    // Legacy OpenGL getters (if needed)
    GLuint getVAO() const {
        return vao_;
    }
    GLuint getVBO() const {
        return vbo_;
    }
    GLuint getEBO() const {
        return ebo_;
    }

    // Get vertex count
    // 11 = 3 position + 3 color + 3 normal + 2 texcoord per vertex
    size_t getVertexCount() const {
        return vertices_.size() / 11;
    }
    size_t getIndexCount() const {
        return indices_.size();
    }

    float CalculateBoundingRadius() const;

   private:
    void setupMesh();

    std::vector<float>        vertices_;
    std::vector<unsigned int> indices_;

    GLuint vao_ = 0;
    GLuint vbo_ = 0;
    GLuint ebo_ = 0;
};
