#pragma once

#include <memory>
#include <string>
#include <vector>

#include "engine/renderer/Material.h"
#include "engine/renderer/Mesh.h"
#include "engine/renderer/Texture.h"

namespace se {

// PBR Material Properties for enhanced model loading
struct PBRMaterialProperties {
    Vector3 Albedo      = Vector3(1.0f);
    float   Metallic    = 0.0f;
    float   Roughness   = 0.5f;
    float   AO          = 1.0f;
    Vector3 Emission    = Vector3(0.0f);
    float   EmissionStr = 0.0f;
    Vector3 Specular    = Vector3(0.5f);
    float   Shininess   = 32.0f;
};

// Texture types for PBR workflow
enum class TextureType { Diffuse, Normal, Metallic, Roughness, AO, Emission, Unknown };

// A SubMesh represents a single drawable mesh with its material
struct SubMesh {
    std::shared_ptr<Mesh>     mesh;
    std::shared_ptr<Material> material;
    PBRMaterialProperties     pbrProperties;

    // Textures stored by type
    std::shared_ptr<Texture> diffuseTexture;
    std::shared_ptr<Texture> normalTexture;
    std::shared_ptr<Texture> metallicTexture;
    std::shared_ptr<Texture> roughnessTexture;
    std::shared_ptr<Texture> aoTexture;
    std::shared_ptr<Texture> emissionTexture;

    float CalculateBoundingRadius() const {
        return mesh ? mesh->CalculateBoundingRadius() : 1.0f;
    }
};

// Model class representing a loaded 3D model with multiple submeshes
class Model {
   public:
    Model() = default;
    explicit Model(const std::string& name) : name_(name) {}

    void AddSubMesh(std::shared_ptr<Mesh> mesh, std::shared_ptr<Material> material) {
        SubMesh submesh;
        submesh.mesh     = mesh;
        submesh.material = material;
        subMeshes_.push_back(std::move(submesh));
    }

    void AddSubMesh(const SubMesh& submesh) {
        subMeshes_.push_back(submesh);
    }

    const std::vector<SubMesh>& GetSubMeshes() const {
        return subMeshes_;
    }
    std::vector<SubMesh>& GetSubMeshes() {
        return subMeshes_;
    }

    size_t GetMeshCount() const {
        return subMeshes_.size();
    }

    const SubMesh& GetSubMesh(size_t index) const {
        return subMeshes_[index];
    }
    SubMesh& GetSubMesh(size_t index) {
        return subMeshes_[index];
    }

    void SetName(const std::string& name) {
        name_ = name;
    }
    const std::string& GetName() const {
        return name_;
    }

    // Apply a material to all submeshes
    void SetMaterialAll(std::shared_ptr<Material> material) {
        for (auto& submesh : subMeshes_) { submesh.material = material; }
    }

    // Calculate total bounding radius (rough approximation)
    float CalculateBoundingRadius() const {
        float maxRadius = 0.0f;
        for (const auto& submesh : subMeshes_) { maxRadius = std::max(maxRadius, submesh.CalculateBoundingRadius()); }
        return maxRadius;
    }

   private:
    std::string          name_;
    std::vector<SubMesh> subMeshes_;
};

// Static texture slot mapping for PBR workflow
inline uint32_t GetTextureSlot(TextureType type) {
    switch (type) {
        case TextureType::Diffuse:
            return 0;
        case TextureType::Normal:
            return 1;
        case TextureType::Metallic:
            return 2;
        case TextureType::Roughness:
            return 3;
        case TextureType::AO:
            return 4;
        case TextureType::Emission:
            return 5;
        default:
            return 0;
    }
}

}  // namespace se
