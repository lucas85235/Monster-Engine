#pragma once

#include <memory>
#include <vector>

#include "engine/renderer/Material.h"
#include "engine/renderer/Mesh.h"

namespace se {

class Model {
   public:
    struct SubMesh {
        std::shared_ptr<Mesh>     mesh;
        std::shared_ptr<Material> material;
    };

    Model() = default;

    void AddSubMesh(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material) {
        subMeshes_.push_back({mesh, material});
    }

    const std::vector<SubMesh>& GetSubMeshes() const {
        return subMeshes_;
    }

   private:
    std::vector<SubMesh> subMeshes_;
};

}  // namespace se
