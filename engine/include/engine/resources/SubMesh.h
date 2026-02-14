#pragma once

#include <memory>
#include <string>

#include "engine/renderer/TextureMaterial.h"

namespace se {

class VertexArray;
class Material;
class MaterialInstance;

class SubMesh {
   public:
    SubMesh() = default;
    SubMesh(std::shared_ptr<VertexArray> vertexArray, 
            std::shared_ptr<Material> material,
            const std::string& name = "");
    ~SubMesh() = default;

    SubMesh(const SubMesh&) = default;
    SubMesh& operator=(const SubMesh&) = default;
    SubMesh(SubMesh&&) noexcept = default;
    SubMesh& operator=(SubMesh&&) noexcept = default;

    void Draw() const;

    const std::shared_ptr<VertexArray>& GetVertexArray() const { return vertexArray_; }
    const std::shared_ptr<Material>& GetMaterial() const { return material_; }
    const std::string& GetName() const { return name_; }

    void SetMaterial(std::shared_ptr<Material> material) { material_ = std::move(material); }

    // Legacy TextureMaterial support (backward compat)
    void SetTextureMaterial(std::shared_ptr<TextureMaterial> texMat) { textureMaterial_ = std::move(texMat); }
    const std::shared_ptr<TextureMaterial>& GetTextureMaterial() const { return textureMaterial_; }
    bool HasTextureMaterial() const { return textureMaterial_ != nullptr; }
    
    // New MaterialInstance support
    void SetMaterialInstance(MaterialInstance* matInstance) { materialInstance_ = matInstance; }
    MaterialInstance* GetMaterialInstance() const { return materialInstance_; }
    bool HasMaterialInstance() const { return materialInstance_ != nullptr; }

   private:
    std::shared_ptr<VertexArray> vertexArray_;
    std::shared_ptr<Material> material_;
    std::shared_ptr<TextureMaterial> textureMaterial_;
    MaterialInstance* materialInstance_ = nullptr;
    std::string name_;
};

}  // namespace se
