#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "engine/resources/ModelData.h"

namespace se {

struct TextureMaterial;
class MaterialInstance;

// A submesh with bone influences for skeletal animation
class SkinnedMesh {
public:
    SkinnedMesh() = default;
    ~SkinnedMesh();
    
    SkinnedMesh(const SkinnedMesh&) = delete;
    SkinnedMesh& operator=(const SkinnedMesh&) = delete;
    SkinnedMesh(SkinnedMesh&& other) noexcept;
    SkinnedMesh& operator=(SkinnedMesh&& other) noexcept;
    
    void Create(const SkinnedSubMeshData& data);
    void Draw() const;
    
    // Legacy TextureMaterial support (backward compat)
    void SetMaterial(std::shared_ptr<TextureMaterial> material) { material_ = material; }
    std::shared_ptr<TextureMaterial> GetMaterial() const { return material_; }
    bool HasMaterial() const { return material_ != nullptr; }
    
    // New MaterialInstance support
    void SetMaterialInstance(MaterialInstance* matInstance) { materialInstance_ = matInstance; }
    MaterialInstance* GetMaterialInstance() const { return materialInstance_; }
    bool HasMaterialInstance() const { return materialInstance_ != nullptr; }
    
    const std::string& GetName() const { return name_; }
    uint32_t GetIndexCount() const { return indexCount_; }
    uint32_t GetVaoId() const { return vaoId_; }
    
private:
    std::string name_;
    uint32_t vaoId_ = 0;
    uint32_t vboId_ = 0;
    uint32_t eboId_ = 0;
    std::shared_ptr<TextureMaterial> material_;
    MaterialInstance* materialInstance_ = nullptr;
    uint32_t indexCount_ = 0;
};

}  // namespace se
