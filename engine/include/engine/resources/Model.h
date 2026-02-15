#pragma once

#include <memory>
#include <string>
#include <vector>

#include "engine/resources/ModelData.h"
#include "engine/resources/SubMesh.h"

namespace se {

class Model {
   public:
    Model() = default;
    explicit Model(const std::string& name);
    ~Model() = default;

    Model(const Model&) = default;
    Model& operator=(const Model&) = default;
    Model(Model&&) noexcept = default;
    Model& operator=(Model&&) noexcept = default;

    void Draw() const;

    void AddSubMesh(SubMesh submesh);

    const std::string& GetName() const {
        return name_;
    }
    const std::vector<SubMesh>& GetSubMeshes() const {
        return subMeshes_;
    }
    size_t GetSubMeshCount() const {
        return subMeshes_.size();
    }

    const BoundingBox& GetBoundingBox() const {
        return bounds_;
    }
    void SetBoundingBox(const BoundingBox& bounds) {
        bounds_ = bounds;
    }

    bool IsValid() const {
        return !subMeshes_.empty();
    }

   private:
    std::string          name_;
    std::vector<SubMesh> subMeshes_;
    BoundingBox          bounds_;
};

}  // namespace se
