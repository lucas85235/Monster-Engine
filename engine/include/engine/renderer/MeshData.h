#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace se {

/**
 * Backend-agnostic vertex data for a single mesh.
 *
 * This is the engine's canonical mesh representation.
 * Rendering backends (Filament, etc.) consume this to create GPU buffers.
 *
 * Layout matches Filament's expected attributes:
 * - POSITION (float3)
 * - TANGENTS (snorm16 quaternion — packed by the backend)
 * - UV0      (float2)
 * - COLOR    (ubyte4)
 */
struct Vertex {
    float position[3] = {0.0f, 0.0f, 0.0f};
    float normal[3]   = {0.0f, 1.0f, 0.0f};
    float uv[2]       = {0.0f, 0.0f};
    float tangent[3]  = {1.0f, 0.0f, 0.0f};
    uint8_t color[4]  = {255, 255, 255, 255};
};

/**
 * Backend-agnostic mesh descriptor.
 *
 * Holds CPU-side vertex and index data that can be uploaded
 * to any rendering backend. Once uploaded, GPU data is owned
 * by the backend — this struct can be discarded.
 */
struct MeshData {
    std::string           name;
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;

    bool IsValid() const {
        return !vertices.empty() && !indices.empty();
    }

    size_t GetVertexCount() const { return vertices.size(); }
    size_t GetIndexCount()  const { return indices.size(); }
};

/**
 * Factory for common mesh primitives.
 *
 * Each method returns a MeshData with proper normals, UVs, and tangents
 * ready for PBR rendering.
 */
namespace MeshPrimitives {

    /** Unit cube centered at origin (side length = 1). */
    MeshData CreateBox(float width = 1.0f, float height = 1.0f, float depth = 1.0f);

    /** UV sphere centered at origin. */
    MeshData CreateSphere(float radius = 0.5f, uint32_t segments = 32, uint32_t rings = 16);

    /** XZ plane centered at origin. */
    MeshData CreatePlane(float width = 1.0f, float depth = 1.0f, uint32_t subdivisions = 1);

    /** Cylinder along Y axis, centered at origin. */
    MeshData CreateCylinder(float radius = 0.5f, float height = 1.0f, uint32_t segments = 32);

} // namespace MeshPrimitives

} // namespace se
