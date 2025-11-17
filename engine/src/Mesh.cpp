#include "engine/Mesh.h"

Mesh::Mesh(const std::vector<float>& vertices, const std::vector<unsigned int>& indices) : vertices_(vertices), indices_(indices) {
    setupMesh();
}

Mesh::Mesh(Mesh&& other) noexcept
    : vertices_(std::move(other.vertices_)), indices_(std::move(other.indices_)), vao_(other.vao_), vbo_(other.vbo_), ebo_(other.ebo_) {
    other.vao_ = 0;
    other.vbo_ = 0;
    other.ebo_ = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this == &other) return *this;

    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (ebo_) glDeleteBuffers(1, &ebo_);

    vertices_ = std::move(other.vertices_);
    indices_  = std::move(other.indices_);
    vao_      = other.vao_;
    vbo_      = other.vbo_;
    ebo_      = other.ebo_;

    other.vao_ = 0;
    other.vbo_ = 0;
    other.ebo_ = 0;

    return *this;
}

Mesh::~Mesh() {
    if (vao_) glDeleteVertexArrays(1, &vao_);
    if (vbo_) glDeleteBuffers(1, &vbo_);
    if (ebo_) glDeleteBuffers(1, &ebo_);
}

void Mesh::setupMesh() {
    // Generate buffers
    glGenVertexArrays(1, &vao_);
    glGenBuffers(1, &vbo_);
    glGenBuffers(1, &ebo_);

    // Bind VAO
    glBindVertexArray(vao_);

    // Bind and set VBO
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, vertices_.size() * sizeof(float), vertices_.data(), GL_STATIC_DRAW);

    // Bind and set EBO
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices_.size() * sizeof(unsigned int), indices_.data(), GL_STATIC_DRAW);

    // Vertex attributes
    // Position attribute (location = 0, 3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Color attribute (location = 1, 3 floats)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Normal attribute (location = 2, 3 floats)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    // Unbind VAO
    glBindVertexArray(0);
}

void Mesh::draw() const {
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(indices_.size()), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}
