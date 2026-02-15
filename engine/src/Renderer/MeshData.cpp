#include "engine/renderer/MeshData.h"

#include <algorithm>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace se {
namespace MeshPrimitives {

namespace {

void EnsureConsistentWinding(MeshData& mesh) {
    if (mesh.indices.size() % 3 != 0) {
        return;
    }

    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        const uint32_t i0 = mesh.indices[i + 0];
        const uint32_t i1 = mesh.indices[i + 1];
        const uint32_t i2 = mesh.indices[i + 2];

        if (i0 >= mesh.vertices.size() || i1 >= mesh.vertices.size() || i2 >= mesh.vertices.size()) {
            continue;
        }

        const auto& v0 = mesh.vertices[i0];
        const auto& v1 = mesh.vertices[i1];
        const auto& v2 = mesh.vertices[i2];

        const float e1x = v1.position[0] - v0.position[0];
        const float e1y = v1.position[1] - v0.position[1];
        const float e1z = v1.position[2] - v0.position[2];

        const float e2x = v2.position[0] - v0.position[0];
        const float e2y = v2.position[1] - v0.position[1];
        const float e2z = v2.position[2] - v0.position[2];

        const float faceNx = e1y * e2z - e1z * e2y;
        const float faceNy = e1z * e2x - e1x * e2z;
        const float faceNz = e1x * e2y - e1y * e2x;

        const float avgNx = v0.normal[0] + v1.normal[0] + v2.normal[0];
        const float avgNy = v0.normal[1] + v1.normal[1] + v2.normal[1];
        const float avgNz = v0.normal[2] + v1.normal[2] + v2.normal[2];

        const float alignment = faceNx * avgNx + faceNy * avgNy + faceNz * avgNz;
        if (alignment < 0.0f) {
            std::swap(mesh.indices[i + 1], mesh.indices[i + 2]);
        }
    }
}

} // namespace

MeshData CreateBox(float width, float height, float depth) {
    MeshData mesh;
    mesh.name = "Box";

    float hw = width  * 0.5f;
    float hh = height * 0.5f;
    float hd = depth  * 0.5f;

    // Helper to add a face (4 vertices, 6 indices)
    auto addFace = [&](
        float p0[3], float p1[3], float p2[3], float p3[3],
        float normal[3], float tangent[3]
    ) {
        uint32_t baseIdx = static_cast<uint32_t>(mesh.vertices.size());

        Vertex v0, v1, v2, v3;
        std::copy(p0, p0 + 3, v0.position); std::copy(normal, normal + 3, v0.normal);
        std::copy(tangent, tangent + 3, v0.tangent);
        v0.uv[0] = 0.0f; v0.uv[1] = 0.0f;

        std::copy(p1, p1 + 3, v1.position); std::copy(normal, normal + 3, v1.normal);
        std::copy(tangent, tangent + 3, v1.tangent);
        v1.uv[0] = 1.0f; v1.uv[1] = 0.0f;

        std::copy(p2, p2 + 3, v2.position); std::copy(normal, normal + 3, v2.normal);
        std::copy(tangent, tangent + 3, v2.tangent);
        v2.uv[0] = 1.0f; v2.uv[1] = 1.0f;

        std::copy(p3, p3 + 3, v3.position); std::copy(normal, normal + 3, v3.normal);
        std::copy(tangent, tangent + 3, v3.tangent);
        v3.uv[0] = 0.0f; v3.uv[1] = 1.0f;

        mesh.vertices.push_back(v0);
        mesh.vertices.push_back(v1);
        mesh.vertices.push_back(v2);
        mesh.vertices.push_back(v3);

        mesh.indices.push_back(baseIdx + 0);
        mesh.indices.push_back(baseIdx + 1);
        mesh.indices.push_back(baseIdx + 2);
        mesh.indices.push_back(baseIdx + 0);
        mesh.indices.push_back(baseIdx + 2);
        mesh.indices.push_back(baseIdx + 3);
    };

    // Front face (+Z)
    { float p0[] = {-hw,-hh, hd}, p1[] = { hw,-hh, hd}, p2[] = { hw, hh, hd}, p3[] = {-hw, hh, hd};
      float n[] = {0,0,1}, t[] = {1,0,0}; addFace(p0,p1,p2,p3,n,t); }
    // Back face (-Z)
    { float p0[] = { hw,-hh,-hd}, p1[] = {-hw,-hh,-hd}, p2[] = {-hw, hh,-hd}, p3[] = { hw, hh,-hd};
      float n[] = {0,0,-1}, t[] = {-1,0,0}; addFace(p0,p1,p2,p3,n,t); }
    // Top face (+Y)
    { float p0[] = {-hw, hh, hd}, p1[] = { hw, hh, hd}, p2[] = { hw, hh,-hd}, p3[] = {-hw, hh,-hd};
      float n[] = {0,1,0}, t[] = {1,0,0}; addFace(p0,p1,p2,p3,n,t); }
    // Bottom face (-Y)
    { float p0[] = {-hw,-hh,-hd}, p1[] = { hw,-hh,-hd}, p2[] = { hw,-hh, hd}, p3[] = {-hw,-hh, hd};
      float n[] = {0,-1,0}, t[] = {1,0,0}; addFace(p0,p1,p2,p3,n,t); }
    // Right face (+X)
    { float p0[] = { hw,-hh, hd}, p1[] = { hw,-hh,-hd}, p2[] = { hw, hh,-hd}, p3[] = { hw, hh, hd};
      float n[] = {1,0,0}, t[] = {0,0,-1}; addFace(p0,p1,p2,p3,n,t); }
    // Left face (-X)
    { float p0[] = {-hw,-hh,-hd}, p1[] = {-hw,-hh, hd}, p2[] = {-hw, hh, hd}, p3[] = {-hw, hh,-hd};
      float n[] = {-1,0,0}, t[] = {0,0,1}; addFace(p0,p1,p2,p3,n,t); }

    EnsureConsistentWinding(mesh);
    return mesh;
}

MeshData CreateSphere(float radius, uint32_t segments, uint32_t rings) {
    MeshData mesh;
    mesh.name = "Sphere";

    for (uint32_t y = 0; y <= rings; ++y) {
        for (uint32_t x = 0; x <= segments; ++x) {
            float xSeg = static_cast<float>(x) / static_cast<float>(segments);
            float ySeg = static_cast<float>(y) / static_cast<float>(rings);

            float theta = xSeg * 2.0f * static_cast<float>(M_PI);
            float phi   = ySeg * static_cast<float>(M_PI);

            float nx = std::sin(phi) * std::cos(theta);
            float ny = std::cos(phi);
            float nz = std::sin(phi) * std::sin(theta);

            Vertex v;
            v.position[0] = nx * radius;
            v.position[1] = ny * radius;
            v.position[2] = nz * radius;
            v.normal[0]   = nx;
            v.normal[1]   = ny;
            v.normal[2]   = nz;
            v.uv[0]       = xSeg;
            v.uv[1]       = ySeg;
            // Tangent: derivative of position w.r.t. theta
            v.tangent[0]  = -std::sin(theta);
            v.tangent[1]  = 0.0f;
            v.tangent[2]  = std::cos(theta);

            mesh.vertices.push_back(v);
        }
    }

    for (uint32_t y = 0; y < rings; ++y) {
        for (uint32_t x = 0; x < segments; ++x) {
            uint32_t i0 = y * (segments + 1) + x;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = (y + 1) * (segments + 1) + x;
            uint32_t i3 = i2 + 1;

            mesh.indices.push_back(i0);
            mesh.indices.push_back(i1);
            mesh.indices.push_back(i2);

            mesh.indices.push_back(i1);
            mesh.indices.push_back(i3);
            mesh.indices.push_back(i2);
        }
    }

    EnsureConsistentWinding(mesh);
    return mesh;
}

MeshData CreatePlane(float width, float depth, uint32_t subdivisions) {
    MeshData mesh;
    mesh.name = "Plane";

    float hw = width * 0.5f;
    float hd = depth * 0.5f;

    for (uint32_t z = 0; z <= subdivisions; ++z) {
        for (uint32_t x = 0; x <= subdivisions; ++x) {
            float u = static_cast<float>(x) / static_cast<float>(subdivisions);
            float v = static_cast<float>(z) / static_cast<float>(subdivisions);

            Vertex vert;
            vert.position[0] = -hw + u * width;
            vert.position[1] = 0.0f;
            vert.position[2] = -hd + v * depth;
            vert.normal[0]   = 0.0f;
            vert.normal[1]   = 1.0f;
            vert.normal[2]   = 0.0f;
            vert.tangent[0]  = 1.0f;
            vert.tangent[1]  = 0.0f;
            vert.tangent[2]  = 0.0f;
            vert.uv[0]       = u;
            vert.uv[1]       = v;

            mesh.vertices.push_back(vert);
        }
    }

    for (uint32_t z = 0; z < subdivisions; ++z) {
        for (uint32_t x = 0; x < subdivisions; ++x) {
            uint32_t i0 = z * (subdivisions + 1) + x;
            uint32_t i1 = i0 + 1;
            uint32_t i2 = (z + 1) * (subdivisions + 1) + x;
            uint32_t i3 = i2 + 1;

            mesh.indices.push_back(i0);
            mesh.indices.push_back(i2);
            mesh.indices.push_back(i1);

            mesh.indices.push_back(i1);
            mesh.indices.push_back(i2);
            mesh.indices.push_back(i3);
        }
    }

    EnsureConsistentWinding(mesh);
    return mesh;
}

MeshData CreateCylinder(float radius, float height, uint32_t segments) {
    MeshData mesh;
    mesh.name = "Cylinder";

    float hh = height * 0.5f;

    // Side vertices
    for (uint32_t i = 0; i <= segments; ++i) {
        float angle = static_cast<float>(i) / static_cast<float>(segments) * 2.0f * static_cast<float>(M_PI);
        float nx    = std::cos(angle);
        float nz    = std::sin(angle);
        float u     = static_cast<float>(i) / static_cast<float>(segments);

        // Bottom vertex
        Vertex vBottom;
        vBottom.position[0] = nx * radius;
        vBottom.position[1] = -hh;
        vBottom.position[2] = nz * radius;
        vBottom.normal[0]   = nx;
        vBottom.normal[1]   = 0.0f;
        vBottom.normal[2]   = nz;
        vBottom.tangent[0]  = -nz;
        vBottom.tangent[1]  = 0.0f;
        vBottom.tangent[2]  = nx;
        vBottom.uv[0]       = u;
        vBottom.uv[1]       = 1.0f;
        mesh.vertices.push_back(vBottom);

        // Top vertex
        Vertex vTop = vBottom;
        vTop.position[1] = hh;
        vTop.uv[1]       = 0.0f;
        mesh.vertices.push_back(vTop);
    }

    // Side indices
    for (uint32_t i = 0; i < segments; ++i) {
        uint32_t b0 = i * 2;
        uint32_t t0 = b0 + 1;
        uint32_t b1 = b0 + 2;
        uint32_t t1 = b0 + 3;

        mesh.indices.push_back(b0);
        mesh.indices.push_back(t0);
        mesh.indices.push_back(b1);

        mesh.indices.push_back(t0);
        mesh.indices.push_back(t1);
        mesh.indices.push_back(b1);
    }

    // Top cap center
    uint32_t topCenter = static_cast<uint32_t>(mesh.vertices.size());
    Vertex centerTop;
    centerTop.position[1] = hh;
    centerTop.normal[1]   = 1.0f;
    centerTop.tangent[0]  = 1.0f;
    mesh.vertices.push_back(centerTop);

    // Top cap ring
    uint32_t topRingStart = static_cast<uint32_t>(mesh.vertices.size());
    for (uint32_t i = 0; i <= segments; ++i) {
        float angle = static_cast<float>(i) / static_cast<float>(segments) * 2.0f * static_cast<float>(M_PI);
        Vertex v;
        v.position[0] = std::cos(angle) * radius;
        v.position[1] = hh;
        v.position[2] = std::sin(angle) * radius;
        v.normal[1]   = 1.0f;
        v.tangent[0]  = 1.0f;
        v.uv[0]       = std::cos(angle) * 0.5f + 0.5f;
        v.uv[1]       = std::sin(angle) * 0.5f + 0.5f;
        mesh.vertices.push_back(v);
    }
    for (uint32_t i = 0; i < segments; ++i) {
        mesh.indices.push_back(topCenter);
        mesh.indices.push_back(topRingStart + i + 1);
        mesh.indices.push_back(topRingStart + i);
    }

    // Bottom cap center
    uint32_t botCenter = static_cast<uint32_t>(mesh.vertices.size());
    Vertex centerBot;
    centerBot.position[1] = -hh;
    centerBot.normal[1]   = -1.0f;
    centerBot.tangent[0]  = 1.0f;
    mesh.vertices.push_back(centerBot);

    // Bottom cap ring
    uint32_t botRingStart = static_cast<uint32_t>(mesh.vertices.size());
    for (uint32_t i = 0; i <= segments; ++i) {
        float angle = static_cast<float>(i) / static_cast<float>(segments) * 2.0f * static_cast<float>(M_PI);
        Vertex v;
        v.position[0] = std::cos(angle) * radius;
        v.position[1] = -hh;
        v.position[2] = std::sin(angle) * radius;
        v.normal[1]   = -1.0f;
        v.tangent[0]  = 1.0f;
        v.uv[0]       = std::cos(angle) * 0.5f + 0.5f;
        v.uv[1]       = std::sin(angle) * 0.5f + 0.5f;
        mesh.vertices.push_back(v);
    }
    for (uint32_t i = 0; i < segments; ++i) {
        mesh.indices.push_back(botCenter);
        mesh.indices.push_back(botRingStart + i);
        mesh.indices.push_back(botRingStart + i + 1);
    }

    EnsureConsistentWinding(mesh);
    return mesh;
}

} // namespace MeshPrimitives
} // namespace se
