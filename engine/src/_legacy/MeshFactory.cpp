#include "engine/MeshFactory.h"

#include <array>
#include <cmath>

namespace {

// Compute normals for vertices with layout: position(3) + uv(2) = 5 floats per vertex BEFORE normals
// After normals are added: position(3) + normal(3) + uv(2) = 8 floats per vertex
static void addNormals(std::vector<float>& vertices, const std::vector<unsigned int>& indices) {
    const size_t stride = 5;  // position(3) + uv(2) before normals
    const size_t vertexCount = vertices.size() / stride;
    std::vector<std::array<float, 3>> normals(vertexCount, {0.f, 0.f, 0.f});

    // Accumulate face normals
    for (size_t i = 0; i + 2 < indices.size(); i += 3) {
        unsigned int i0 = indices[i];
        unsigned int i1 = indices[i + 1];
        unsigned int i2 = indices[i + 2];

        float x0 = vertices[i0 * stride], y0 = vertices[i0 * stride + 1], z0 = vertices[i0 * stride + 2];
        float x1 = vertices[i1 * stride], y1 = vertices[i1 * stride + 1], z1 = vertices[i1 * stride + 2];
        float x2 = vertices[i2 * stride], y2 = vertices[i2 * stride + 1], z2 = vertices[i2 * stride + 2];

        float ax = x1 - x0, ay = y1 - y0, az = z1 - z0;
        float bx = x2 - x0, by = y2 - y0, bz = z2 - z0;

        float nx = ay * bz - az * by;
        float ny = az * bx - ax * bz;
        float nz = ax * by - ay * bx;

        normals[i0][0] += nx; normals[i0][1] += ny; normals[i0][2] += nz;
        normals[i1][0] += nx; normals[i1][1] += ny; normals[i1][2] += nz;
        normals[i2][0] += nx; normals[i2][1] += ny; normals[i2][2] += nz;
    }

    // Create new vertex array: position(3) + normal(3) + uv(2) = 8 floats
    // This matches model.vert layout: a_Position(0), a_Normal(1), a_TexCoord(2)
    std::vector<float> newVerts;
    newVerts.reserve(vertexCount * 8);
    for (size_t i = 0; i < vertexCount; ++i) {
        // Position
        float px = vertices[i * stride];
        float py = vertices[i * stride + 1];
        float pz = vertices[i * stride + 2];
        newVerts.push_back(px);
        newVerts.push_back(py);
        newVerts.push_back(pz);

        // Normalize normal
        float nx = normals[i][0];
        float ny = normals[i][1];
        float nz = normals[i][2];
        float length = std::sqrt(nx * nx + ny * ny + nz * nz);
        if (length > 1e-6f) {
            nx /= length;
            ny /= length;
            nz /= length;
        } else {
            // Fallback: use position as normal (works for centered sphere)
            float pl = std::sqrt(px*px + py*py + pz*pz);
            if (pl > 1e-6f) {
                nx = px / pl; ny = py / pl; nz = pz / pl;
            } else {
                nx = 0.0f; ny = 1.0f; nz = 0.0f;
            }
        }
        newVerts.push_back(nx);
        newVerts.push_back(ny);
        newVerts.push_back(nz);

        // UV (at the end to match model.vert layout)
        newVerts.push_back(vertices[i * stride + 3]);
        newVerts.push_back(vertices[i * stride + 4]);
    }

    vertices.swap(newVerts);
}

}  // namespace

// Helper: Add vertex with position + UV (normals added later)
void MeshFactory::addVertex(std::vector<float>& vertices, float x, float y, float z, float u, float v) {
    vertices.push_back(x);
    vertices.push_back(y);
    vertices.push_back(z);
    vertices.push_back(u);
    vertices.push_back(v);
}

Mesh MeshFactory::CreateTriangle() {
    std::vector<float> vertices = {
        // positions        // UV
        0.0f,  0.5f,  0.0f, 0.5f, 1.0f,  // top
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,  // bottom left
        0.5f,  -0.5f, 0.0f, 1.0f, 0.0f   // bottom right
    };

    std::vector<unsigned int> indices = {0, 1, 2};

    addNormals(vertices, indices);

    return Mesh(vertices, indices);
}

Mesh MeshFactory::CreateQuad() {
    std::vector<float> vertices = {
        // positions        // UV
        -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,  // bottom left
        0.5f,  -0.5f, 0.0f, 1.0f, 0.0f,  // bottom right
        0.5f,  0.5f,  0.0f, 1.0f, 1.0f,  // top right
        -0.5f, 0.5f,  0.0f, 0.0f, 1.0f   // top left
    };

    std::vector<unsigned int> indices = {
        0, 1, 2,
        2, 3, 0
    };

    addNormals(vertices, indices);

    return Mesh(vertices, indices);
}

Mesh MeshFactory::CreateCube() {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // Each face has 4 vertices with proper UV mapping (0,0) to (1,1)
    // Front face (+Z)
    addVertex(vertices, -0.5f, -0.5f, 0.5f, 0.0f, 0.0f);  // 0
    addVertex(vertices, 0.5f, -0.5f, 0.5f, 1.0f, 0.0f);   // 1
    addVertex(vertices, 0.5f, 0.5f, 0.5f, 1.0f, 1.0f);    // 2
    addVertex(vertices, -0.5f, 0.5f, 0.5f, 0.0f, 1.0f);   // 3

    // Back face (-Z)
    addVertex(vertices, 0.5f, -0.5f, -0.5f, 0.0f, 0.0f);  // 4
    addVertex(vertices, -0.5f, -0.5f, -0.5f, 1.0f, 0.0f); // 5
    addVertex(vertices, -0.5f, 0.5f, -0.5f, 1.0f, 1.0f);  // 6
    addVertex(vertices, 0.5f, 0.5f, -0.5f, 0.0f, 1.0f);   // 7

    // Top face (+Y)
    addVertex(vertices, -0.5f, 0.5f, 0.5f, 0.0f, 0.0f);   // 8
    addVertex(vertices, 0.5f, 0.5f, 0.5f, 1.0f, 0.0f);    // 9
    addVertex(vertices, 0.5f, 0.5f, -0.5f, 1.0f, 1.0f);   // 10
    addVertex(vertices, -0.5f, 0.5f, -0.5f, 0.0f, 1.0f);  // 11

    // Bottom face (-Y)
    addVertex(vertices, -0.5f, -0.5f, -0.5f, 0.0f, 0.0f); // 12
    addVertex(vertices, 0.5f, -0.5f, -0.5f, 1.0f, 0.0f);  // 13
    addVertex(vertices, 0.5f, -0.5f, 0.5f, 1.0f, 1.0f);   // 14
    addVertex(vertices, -0.5f, -0.5f, 0.5f, 0.0f, 1.0f);  // 15

    // Right face (+X)
    addVertex(vertices, 0.5f, -0.5f, 0.5f, 0.0f, 0.0f);   // 16
    addVertex(vertices, 0.5f, -0.5f, -0.5f, 1.0f, 0.0f);  // 17
    addVertex(vertices, 0.5f, 0.5f, -0.5f, 1.0f, 1.0f);   // 18
    addVertex(vertices, 0.5f, 0.5f, 0.5f, 0.0f, 1.0f);    // 19

    // Left face (-X)
    addVertex(vertices, -0.5f, -0.5f, -0.5f, 0.0f, 0.0f); // 20
    addVertex(vertices, -0.5f, -0.5f, 0.5f, 1.0f, 0.0f);  // 21
    addVertex(vertices, -0.5f, 0.5f, 0.5f, 1.0f, 1.0f);   // 22
    addVertex(vertices, -0.5f, 0.5f, -0.5f, 0.0f, 1.0f);  // 23

    unsigned int cubeIndices[] = {
        0,  1,  2,  2,  3,  0,   // front
        4,  5,  6,  6,  7,  4,   // back
        8,  9,  10, 10, 11, 8,   // top
        12, 13, 14, 14, 15, 12,  // bottom
        16, 17, 18, 18, 19, 16,  // right
        20, 21, 22, 22, 23, 20   // left
    };

    indices.assign(cubeIndices, cubeIndices + 36);

    addNormals(vertices, indices);

    return Mesh(vertices, indices);
}

Mesh MeshFactory::CreateSphere(int segments, int rings) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    const float PI = 3.14159265359f;
    float radius = 0.5f;

    for (int ring = 0; ring <= rings; ring++) {
        float theta = ring * PI / rings;
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);

        for (int seg = 0; seg <= segments; seg++) {
            float phi = seg * 2.0f * PI / segments;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            float x = cosPhi * sinTheta;
            float y = cosTheta;
            float z = sinPhi * sinTheta;

            // Spherical UV mapping (U and V inverted for correct orientation)
            float u = 1.0f - static_cast<float>(seg) / segments;
            float v = 1.0f - static_cast<float>(ring) / rings;

            addVertex(vertices, x * radius, y * radius, z * radius, u, v);
        }
    }

    for (int ring = 0; ring < rings; ring++) {
        for (int seg = 0; seg < segments; seg++) {
            int current = ring * (segments + 1) + seg;
            int next = current + segments + 1;

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
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    const float PI = 3.14159265359f;
    float halfHeight = height * 0.5f;

    int ringsPerCap = segments / 2;
    int totalRings = segments + 1;

    // Top hemisphere
    for (int ring = 0; ring <= ringsPerCap; ring++) {
        float theta = ring * PI / segments;
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);

        for (int seg = 0; seg <= segments; seg++) {
            float phi = seg * 2.0f * PI / segments;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            float x = cosPhi * sinTheta * radius;
            float y = cosTheta * radius + halfHeight;
            float z = sinPhi * sinTheta * radius;

            float u = 1.0f - static_cast<float>(seg) / segments;
            float v = 1.0f - static_cast<float>(ring) / totalRings;

            addVertex(vertices, x, y, z, u, v);
        }
    }

    // Bottom hemisphere
    for (int ring = ringsPerCap + 1; ring <= segments; ring++) {
        float theta = ring * PI / segments;
        float sinTheta = std::sin(theta);
        float cosTheta = std::cos(theta);

        for (int seg = 0; seg <= segments; seg++) {
            float phi = seg * 2.0f * PI / segments;
            float sinPhi = std::sin(phi);
            float cosPhi = std::cos(phi);

            float x = cosPhi * sinTheta * radius;
            float y = cosTheta * radius - halfHeight;
            float z = sinPhi * sinTheta * radius;

            float u = 1.0f - static_cast<float>(seg) / segments;
            float v = 1.0f - static_cast<float>(ring) / totalRings;

            addVertex(vertices, x, y, z, u, v);
        }
    }

    for (int ring = 0; ring < totalRings - 1; ring++) {
        for (int seg = 0; seg < segments; seg++) {
            int current = ring * (segments + 1) + seg;
            int next = current + segments + 1;

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
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    const float PI = 3.14159265359f;
    float halfHeight = height * 0.5f;

    // Top circle (for side faces)
    for (int i = 0; i <= segments; i++) {
        float angle = i * 2.0f * PI / segments;
        float x = std::cos(angle) * radius;
        float z = std::sin(angle) * radius;
        float u = static_cast<float>(i) / segments;
        addVertex(vertices, x, halfHeight, z, u, 1.0f);
    }

    // Bottom circle (for side faces)
    for (int i = 0; i <= segments; i++) {
        float angle = i * 2.0f * PI / segments;
        float x = std::cos(angle) * radius;
        float z = std::sin(angle) * radius;
        float u = static_cast<float>(i) / segments;
        addVertex(vertices, x, -halfHeight, z, u, 0.0f);
    }

    // Side indices
    for (int i = 0; i < segments; i++) {
        int top1 = i;
        int top2 = i + 1;
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
