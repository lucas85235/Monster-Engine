#pragma once

#include <glm.hpp>
#include <memory>
#include <string>
#include <vector>

namespace se {

struct ModelVertex {
    glm::vec3 Position{0.0f};
    glm::vec3 Normal{0.0f, 1.0f, 0.0f};
    glm::vec2 TexCoord{0.0f};
    glm::vec3 Tangent{1.0f, 0.0f, 0.0f};
    glm::vec3 Bitangent{0.0f, 0.0f, 1.0f};
};

struct MaterialData {
    std::string Name;
    
    glm::vec4 DiffuseColor{1.0f};
    glm::vec4 SpecularColor{1.0f};
    glm::vec4 AmbientColor{0.1f, 0.1f, 0.1f, 1.0f};
    glm::vec3 EmissiveColor{0.0f};
    float Shininess = 32.0f;
    float Metallic = 0.0f;
    float Roughness = 0.5f;
    
    std::string DiffuseTexturePath;
    std::string NormalTexturePath;
    std::string SpecularTexturePath;
    std::string AOTexturePath;
    std::string EmissiveTexturePath;
    std::string RoughnessTexturePath;
    std::string MetallicTexturePath;
};

struct SubMeshData {
    std::vector<ModelVertex> Vertices;
    std::vector<uint32_t> Indices;
    int MaterialIndex = -1;
    std::string Name;
};

struct BoundingBox {
    glm::vec3 Min{0.0f};
    glm::vec3 Max{0.0f};
    
    glm::vec3 GetCenter() const { return (Min + Max) * 0.5f; }
    glm::vec3 GetExtents() const { return (Max - Min) * 0.5f; }
};

struct ModelData {
    std::string Name;
    std::string SourcePath;
    
    std::vector<SubMeshData> SubMeshes;
    std::vector<MaterialData> Materials;
    
    BoundingBox Bounds;
    
    bool IsValid() const { return !SubMeshes.empty(); }
};

}  // namespace se
