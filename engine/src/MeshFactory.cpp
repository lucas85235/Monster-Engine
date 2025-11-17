#include "engine/MeshFactory.h"

#include <array>
#include <cmath>

// Anonymous namespace to hold helper for computing normals
namespace {
static void addNormals(std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
    const size_t                      vertexCount = vertices.size() / 6;  // pos+color per vertex
    std::vector<std::array<float, 3>> normals(vertexCount, {0.f, 0.f, 0.f});

    // accumulate face normals
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];

        // positions of triangle vertices
        float x0 = vertices[i0 * 6], y0 = vertices[i0 * 6 + 1], z0 = vertices[i0 * 6 + 2];
        float x1 = vertices[i1 * 6], y1 = vertices[i1 * 6 + 1], z1 = vertices[i1 * 6 + 2];
        float x2 = vertices[i2 * 6], y2 = vertices[i2 * 6 + 1], z2 = vertices[i2 * 6 + 2];

        // edges
        float ax = x1 - x0;
        float ay = y1 - y0;
        float az = z1 - z0;
        float bx = x2 - x0;
        float by = y2 - y0;
        float bz = z2 - z0;

        // cross product
        float nx = ay * bz - az * by;
        float ny = az * bx - ax * bz;
        float nz = ax * by - ay * bx;

        // accumulate normals for each vertex of triangle
        normals[i0][0] += nx;
        normals[i0][1] += ny;
        normals[i0][2] += nz;
        normals[i1][0] += nx;
        normals[i1][1] += ny;
        normals[i1][2] += nz;
        normals[i2][0] += nx;
        normals[i2][1] += ny;
        normals[i2][2] += nz;
    }

    // create new interleaved vertex array with normals
    std::vector<float> newVerts;
    newVerts.reserve(vertexCount * 9);
    for (size_t i = 0; i < vertexCount; ++i) {
        // copy position and color
        newVerts.push_back(vertices[i * 6]);
        newVerts.push_back(vertices[i * 6 + 1]);
        newVerts.push_back(vertices[i * 6 + 2]);
        newVerts.push_back(vertices[i * 6 + 3]);
        newVerts.push_back(vertices[i * 6 + 4]);
        newVerts.push_back(vertices[i * 6 + 5]);

        // normalize normal
        float nx     = normals[i][0];
        float ny     = normals[i][1];
        float nz     = normals[i][2];
        float length = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (length > 0.f) {
            nx /= length;
            ny /= length;
            nz /= length;
        }
        newVerts.push_back(nx);
        newVerts.push_back(ny);
        newVerts.push_back(nz);
    }

    vertices.swap(newVerts);
}
}  // namespace

void MeshFactory::addVertex(std::vector<float>& vertices, float x, float y, float z, float r, float g, float b) {
    vertices.push_back(x);
    vertices.push_back(y);
    vertices.push_back(z);
    vertices.push_back(r);
    vertices.push_back(g);
    vertices.push_back(b);
}

Mesh MeshFactory::CreateTriangle() {
    std::vector<float> vertices = {
        // positions        // colors
        0.0f,  0.5f,  0.0f, 1.0f, 0.0f, 0.0f,  // top - red
        -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,  // bottom left - green
        0.5f,  -0.5f, 0.0f, 0.0f, 0.0f, 1.0f   // bottom right - blue
    };

    std::vector<unsigned int> indices = {0, 1, 2};

    addNormals(vertices, indices);

    return Mesh(vertices, indices);
}

Mesh MeshFactory::CreateQuad() {
    std::vector<float> vertices = {
        // positions        // colors
        -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f,  // bottom left - red
        0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f,  // bottom right - green
        0.5f,  0.5f,  0.0f, 0.0f, 0.0f, 1.0f,  // top right - blue
        -0.5f, 0.5f,  0.0f, 1.0f, 1.0f, 0.0f   // top left - yellow
    };

    std::vector<unsigned int> indices = {
        0, 1, 2,  // first triangle
        2, 3, 0   // second triangle
    };

    addNormals(vertices, indices);

    return Mesh(vertices, indices);
}

Mesh MeshFactory::CreateCube() {
    std::vector<float>        vertices;
    std::vector<unsigned int> indices;

    // Front face (z = 0.5)
    addVertex(vertices, -0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f);  // 0
    addVertex(vertices, 0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f);   // 1
    addVertex(vertices, 0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f);    // 2
    addVertex(vertices, -0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f);   // 3

    // Back face (z = -0.5)
    addVertex(vertices, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f);  // 4
    addVertex(vertices, 0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f);   // 5
    addVertex(vertices, 0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f);    // 6
    addVertex(vertices, -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f);   // 7

    // Top face (y = 0.5)
    addVertex(vertices, -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f);   // 8
    addVertex(vertices, 0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f);    // 9
    addVertex(vertices, 0.5f, 0.5f, -0.5f, 0.0f, 0.0f, 1.0f);   // 10
    addVertex(vertices, -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, 1.0f);  // 11

    // Bottom face (y = -0.5)
    addVertex(vertices, -0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 0.0f);   // 12
    addVertex(vertices, 0.5f, -0.5f, 0.5f, 1.0f, 1.0f, 0.0f);    // 13
    addVertex(vertices, 0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f);   // 14
    addVertex(vertices, -0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f);  // 15

    // Right face (x = 0.5)
    addVertex(vertices, 0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f);   // 16
    addVertex(vertices, 0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 1.0f);  // 17
    addVertex(vertices, 0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 1.0f);   // 18
    addVertex(vertices, 0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 1.0f);    // 19

    // Left face (x = -0.5)
    addVertex(vertices, -0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 1.0f);   // 20
    addVertex(vertices, -0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 1.0f);  // 21
    addVertex(vertices, -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 1.0f);   // 22
    addVertex(vertices, -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 1.0f);    // 23

    // Indices for all faces
    unsigned int cubeIndices[] = {
        0,  1,  2,  2,  3,  0,   // front (+Z)
        4,  6,  5,  4,  7,  6,   // back (-Z)
        8,  9,  10, 10, 11, 8,   // top (+Y)
        12, 14, 13, 12, 15, 14,  // bottom (-Y)
        16, 17, 18, 18, 19, 16,  // right (+X)
        20, 22, 21, 20, 23, 22   // left (-X)
    };

    indices.assign(cubeIndices, cubeIndices + 36);

    addNormals(vertices, indices);

    return Mesh(vertices, indices);
}

Mesh MeshFactory::CreateSphere(int segments, int rings) {
    std::vector<float>        vertices;
    std::vector<unsigned int> indices;

    const float PI     = 3.14159265359f;
    float       radius = 0.5f;

    // Generate vertices
    for (int ring = 0; ring <= rings; ring++) {
        float theta    = ring * PI / rings;
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);

        for (int seg = 0; seg <= segments; seg++) {
            float phi    = seg * 2.0f * PI / segments;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;

            float r = (x + 1.0f) * 0.5f;
            float g = (y + 1.0f) * 0.5f;
            float b = (z + 1.0f) * 0.5f;

            addVertex(vertices, x * radius, y * radius, z * radius, r, g, b);
        }
    }

    // Generate indices
    for (int ring = 0; ring < rings; ring++) {
        for (int seg = 0; seg < segments; seg++) {
            int current = ring * (segments + 1) + seg;
            int next    = current + segments + 1;

            indices.push_back(current);
            indices.push_back(current + 1);
            indices.push_back(next);

            indices.push_back(current + 1);
            indices.push_back(next + 1);
            indices.push_back(next);
        }
    }

    addNormals(vertices, indices);

    return Mesh(vertices, indices);
}

Mesh MeshFactory::CreateCapsule(float radius, float height, int segments) {
    std::vector<float>        vertices;
    std::vector<unsigned int> indices;

    const float PI         = 3.14159265359f;
    float       halfHeight = height * 0.5f;

    // Top hemisphere
    for (int ring = 0; ring <= segments / 2; ring++) {
        float theta    = ring * PI / segments;
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);

        for (int seg = 0; seg <= segments; seg++) {
            float phi    = seg * 2.0f * PI / segments;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            float x = cosPhi * sinTheta * radius;
            float y = cosTheta * radius + halfHeight;
            float z = sinPhi * sinTheta * radius;

            float r = (cosPhi + 1.0f) * 0.5f;
            float g = (sinPhi + 1.0f) * 0.5f;
            float b = (cosTheta + 1.0f) * 0.5f;

            addVertex(vertices, x, y, z, r, g, b);
        }
    }

    // Bottom hemisphere
    for (int ring = segments / 2 + 1; ring <= segments; ring++) {
        float theta    = ring * PI / segments;
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);

        for (int seg = 0; seg <= segments; seg++) {
            float phi    = seg * 2.0f * PI / segments;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            float x = cosPhi * sinTheta * radius;
            float y = cosTheta * radius - halfHeight;
            float z = sinPhi * sinTheta * radius;

            float r = (cosPhi + 1.0f) * 0.5f;
            float g = (sinPhi + 1.0f) * 0.5f;
            float b = (cosTheta + 1.0f) * 0.5f;

            addVertex(vertices, x, y, z, r, g, b);
        }
    }

    int ringsCount = segments + 1;
    for (int ring = 0; ring < ringsCount - 1; ring++) {
        for (int seg = 0; seg < segments; seg++) {
            int current = ring * (segments + 1) + seg;
            int next    = current + segments + 1;

            indices.push_back(current);
            indices.push_back(current + 1);
            indices.push_back(next);

            indices.push_back(current + 1);
            indices.push_back(next + 1);
            indices.push_back(next);
        }
    }

    addNormals(vertices, indices);

    return Mesh(vertices, indices);
}

Mesh MeshFactory::CreateCylinder(float radius, float height, int segments) {
    std::vector<float>        vertices;
    std::vector<unsigned int> indices;

    const float PI         = 3.14159265359f;
    float       halfHeight = height * 0.5f;

    // Top circle
    for (int i = 0; i <= segments; i++) {
        float angle = i * 2.0f * PI / segments;
        float x     = std::cos(angle) * radius;
        float z     = std::sin(angle) * radius;
        addVertex(vertices, x, halfHeight, z, 1.0f, 0.0f, 0.0f);
    }

    // Bottom circle
    for (int i = 0; i <= segments; i++) {
        float angle = i * 2.0f * PI / segments;
        float x     = std::cos(angle) * radius;
        float z     = std::sin(angle) * radius;
        addVertex(vertices, x, -halfHeight, z, 0.0f, 1.0f, 0.0f);
    }

    // Side indices
    for (int i = 0; i < segments; i++) {
        int top1    = i;
        int top2    = i + 1;
        int bottom1 = i + segments + 1;
        int bottom2 = i + segments + 2;

        indices.push_back(top1);
        indices.push_back(top2);
        indices.push_back(bottom1);

        indices.push_back(top2);
        indices.push_back(bottom2);
        indices.push_back(bottom1);
    }

    addNormals(vertices, indices);

    return Mesh(vertices, indices);
}
