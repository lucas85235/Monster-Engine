#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include "engine/Mesh.h"
#include "engine/renderer/VertexArray.h"

namespace se {
enum class PrimitiveMeshType { Triangle, Quad, Cube, Sphere, Capsule, Cylinder };

class MeshManager {
   public:
    MeshManager() = delete;

    static void Init();

    static void Shutdown();

    // Create a vertex array from a Mesh object
    static std::shared_ptr<VertexArray> CreateVertexArrayFromMesh(const Mesh& mesh);

    // Get or create primitive mesh (cached)
    static std::shared_ptr<VertexArray> GetPrimitive(PrimitiveMeshType type);

    // Clear all cached meshes
    static void ClearCache();

   private:
    static std::shared_ptr<VertexArray> CreatePrimitive(PrimitiveMeshType type);

    static std::unordered_map<PrimitiveMeshType, std::shared_ptr<VertexArray>> primitiveCache_;
    static bool                                                                initialized_;
};
}  // namespace se
