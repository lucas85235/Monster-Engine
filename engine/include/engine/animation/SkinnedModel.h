#pragma once

#include <memory>
#include <string>
#include <vector>

#include "engine/animation/SkinnedMesh.h"
#include "engine/resources/ModelData.h"

namespace se {

// A model with skeleton and skinned meshes for animation
class SkinnedModel {
public:
    SkinnedModel() = default;
    explicit SkinnedModel(const std::string& name);
    ~SkinnedModel() = default;
    
    void Draw() const;
    
    void AddMesh(SkinnedMesh mesh);
    
    const std::string& GetName() const { return name_; }
    const std::vector<SkinnedMesh>& GetMeshes() const { return meshes_; }
    std::vector<SkinnedMesh>& GetMeshes() { return meshes_; }
    size_t GetMeshCount() const { return meshes_.size(); }
    
    void SetModelData(std::shared_ptr<SkinnedModelData> data) { modelData_ = data; }
    std::shared_ptr<SkinnedModelData> GetModelData() const { return modelData_; }
    
    bool HasSkeleton() const { return modelData_ && modelData_->HasSkeleton(); }
    
    const BoundingBox& GetBoundingBox() const { return bounds_; }
    void SetBoundingBox(const BoundingBox& bounds) { bounds_ = bounds; }
    
    bool IsValid() const { return !meshes_.empty(); }
    
private:
    std::string name_;
    std::vector<SkinnedMesh> meshes_;
    std::shared_ptr<SkinnedModelData> modelData_;
    BoundingBox bounds_;
};

}  // namespace se
