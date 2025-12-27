#pragma once

#include <memory>
#include <string>

namespace se {

class VertexArray;
class Material;

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

   private:
    std::shared_ptr<VertexArray> vertexArray_;
    std::shared_ptr<Material> material_;
    std::string name_;
};

}  // namespace se
