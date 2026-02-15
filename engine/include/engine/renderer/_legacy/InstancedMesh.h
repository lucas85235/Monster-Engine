#pragma once

#include <glm.hpp>
#include <memory>
#include <vector>

#include "engine/renderer/Buffer.h"
#include "engine/renderer/IInstanceBuffer.h"
#include "engine/renderer/Material.h"
#include "engine/renderer/VertexArray.h"

namespace se {

/**
 * Per-instance data structure.
 * Contains transform matrix and color for each instance.
 * Can be extended with additional per-instance attributes.
 */
struct InstanceData {
    Matrix4 Transform{1.0f};
    Vector4 Color{1.0f, 1.0f, 1.0f, 1.0f};

    static BufferLayout GetLayout() {
        return {{ShaderDataType::Mat4, "a_InstanceTransform", 1u},
                {ShaderDataType::Float4, "a_InstanceColor", 1u}};
    }

    static constexpr uint32_t GetStride() {
        return sizeof(Matrix4) + sizeof(Vector4);
    }
};

/**
 * InstancedMesh - Renders many copies of the same mesh efficiently.
 *
 * Uses GPU instancing to reduce draw calls when rendering multiple
 * copies of the same geometry with different transforms/colors.
 *
 * Usage:
 *   auto instancedMesh = std::make_shared<InstancedMesh>(baseVertexArray, 1000);
 *   std::vector<InstanceData> instances(100);
 *   // ... fill instances with transforms and colors ...
 *   instancedMesh->SetInstances(instances);
 *   instancedMesh->Draw(material);
 */
class InstancedMesh {
   public:
    InstancedMesh(const std::shared_ptr<VertexArray>& baseVA, uint32_t maxInstances);
    ~InstancedMesh() = default;

    InstancedMesh(const InstancedMesh&)            = delete;
    InstancedMesh& operator=(const InstancedMesh&) = delete;

    void SetInstances(const std::vector<InstanceData>& instances);
    void UpdateInstance(uint32_t index, const InstanceData& data);
    void Draw(const std::shared_ptr<Material>& material);
    void DrawWithoutMaterial();

    uint32_t GetInstanceCount() const {
        return currentInstanceCount_;
    }
    uint32_t GetMaxInstances() const {
        return maxInstances_;
    }

    const std::shared_ptr<VertexArray>& GetVertexArray() const {
        return instancedVA_;
    }

   private:
    std::shared_ptr<VertexArray>     instancedVA_;
    std::shared_ptr<IInstanceBuffer> instanceBuffer_;
    uint32_t                         maxInstances_         = 0;
    uint32_t                         currentInstanceCount_ = 0;
};

}  // namespace se
