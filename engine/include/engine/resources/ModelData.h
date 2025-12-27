#pragma once

#include <glm.hpp>
#include <gtc/quaternion.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace se {

// Animation system constants
constexpr int MAX_BONE_INFLUENCE = 4;
constexpr int MAX_BONES = 128;

// Bone information for skeletal hierarchy
struct BoneInfo {
    std::string Name;
    int Id = -1;
    int ParentIndex = -1;
    glm::mat4 OffsetMatrix{1.0f};   // Inverse bind pose matrix  
    glm::mat4 LocalTransform{1.0f}; // Node's local transform from hierarchy
};

// Standard vertex for static meshes
struct ModelVertex {
    glm::vec3 Position{0.0f};
    glm::vec3 Normal{0.0f, 1.0f, 0.0f};
    glm::vec2 TexCoord{0.0f};
    glm::vec3 Tangent{1.0f, 0.0f, 0.0f};
    glm::vec3 Bitangent{0.0f, 0.0f, 1.0f};
};

// Vertex with bone influences for skeletal animation
struct SkinnedVertex {
    glm::vec3 Position{0.0f};
    glm::vec3 Normal{0.0f, 1.0f, 0.0f};
    glm::vec2 TexCoord{0.0f};
    glm::vec3 Tangent{1.0f, 0.0f, 0.0f};
    glm::vec3 Bitangent{0.0f, 0.0f, 1.0f};
    int BoneIds[MAX_BONE_INFLUENCE] = {-1, -1, -1, -1};
    float BoneWeights[MAX_BONE_INFLUENCE] = {0.0f, 0.0f, 0.0f, 0.0f};

    void AddBoneData(int boneId, float weight) {
        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
            if (BoneIds[i] < 0) {
                BoneIds[i] = boneId;
                BoneWeights[i] = weight;
                return;
            }
        }
    }

    void NormalizeWeights() {
        float total = 0.0f;
        for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
            if (BoneIds[i] >= 0) total += BoneWeights[i];
        }
        if (total > 0.0f) {
            for (int i = 0; i < MAX_BONE_INFLUENCE; ++i) {
                if (BoneIds[i] >= 0) BoneWeights[i] /= total;
            }
        }
    }
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

// Submesh with skinning data
struct SkinnedSubMeshData {
    std::vector<SkinnedVertex> Vertices;
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

// Model data with skeletal animation support
struct SkinnedModelData {
    std::string Name;
    std::string SourcePath;
    
    std::vector<SkinnedSubMeshData> SubMeshes;
    std::vector<MaterialData> Materials;
    std::vector<BoneInfo> Bones;
    std::unordered_map<std::string, int> BoneNameToIndex;
    glm::mat4 GlobalInverseTransform{1.0f};
    
    BoundingBox Bounds;
    
    bool IsValid() const { return !SubMeshes.empty(); }
    bool HasSkeleton() const { return !Bones.empty(); }
    
    int GetBoneIndex(const std::string& name) const {
        auto it = BoneNameToIndex.find(name);
        return it != BoneNameToIndex.end() ? it->second : -1;
    }
};

}  // namespace se

