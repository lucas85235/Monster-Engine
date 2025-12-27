#include "engine/animation/SkinnedModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <filesystem>

#include "engine/Log.h"

namespace se {

namespace {
glm::mat4 ConvertMatrix(const aiMatrix4x4& m) {
    return glm::mat4(
        m.a1, m.b1, m.c1, m.d1,
        m.a2, m.b2, m.c2, m.d2,
        m.a3, m.b3, m.c3, m.d3,
        m.a4, m.b4, m.c4, m.d4
    );
}

std::string FindTexture(const aiMaterial* material, aiTextureType type, const std::string& directory) {
    aiString texPath;
    if (material->GetTexture(type, 0, &texPath) == AI_SUCCESS) {
        std::filesystem::path fullPath = std::filesystem::path(directory) / texPath.C_Str();
        if (std::filesystem::exists(fullPath)) {
            return fullPath.string();
        }
    }
    return "";
}
}  // namespace

std::unique_ptr<SkinnedModelData> SkinnedModelLoader::Load(const std::string& path) {
    SE_LOG_INFO("SkinnedModelLoader: Loading '{}'", path);
    
    Assimp::Importer importer;
    
    unsigned int flags = aiProcess_Triangulate |
                         aiProcess_GenNormals |
                         aiProcess_FlipUVs |
                         aiProcess_CalcTangentSpace |
                         aiProcess_LimitBoneWeights |
                         aiProcess_SortByPType;
    
    const aiScene* scene = importer.ReadFile(path, flags);
    
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        SE_LOG_ERROR("SkinnedModelLoader: Failed to load '{}': {}", path, importer.GetErrorString());
        return nullptr;
    }
    
    auto data = std::make_unique<SkinnedModelData>();
    data->SourcePath = path;
    
    std::filesystem::path filePath(path);
    data->Name = filePath.stem().string();
    std::string directory = filePath.parent_path().string();
    
    data->GlobalInverseTransform = glm::inverse(ConvertMatrix(scene->mRootNode->mTransformation));
    
    // Process materials
    for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
        ProcessMaterial(scene->mMaterials[i], *data, directory);
    }
    
    // Process nodes and meshes
    ProcessNode(scene->mRootNode, scene, *data, directory);
    
    // Build bone hierarchy
    ProcessBoneHierarchy(scene->mRootNode, *data, -1);
    
    // Calculate bounds
    data->Bounds.Min = glm::vec3(std::numeric_limits<float>::max());
    data->Bounds.Max = glm::vec3(std::numeric_limits<float>::lowest());
    
    for (const auto& submesh : data->SubMeshes) {
        for (const auto& vertex : submesh.Vertices) {
            data->Bounds.Min = glm::min(data->Bounds.Min, vertex.Position);
            data->Bounds.Max = glm::max(data->Bounds.Max, vertex.Position);
        }
    }
    
    SE_LOG_INFO("SkinnedModelLoader: Loaded '{}' - {} submeshes, {} bones, {} materials",
                data->Name, data->SubMeshes.size(), data->Bones.size(), data->Materials.size());
    
    return data;
}

void SkinnedModelLoader::ProcessNode(const void* nodePtr, const void* scenePtr, SkinnedModelData& data, const std::string& directory) {
    const aiNode* node = static_cast<const aiNode*>(nodePtr);
    const aiScene* scene = static_cast<const aiScene*>(scenePtr);
    
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessMesh(mesh, scene, data);
    }
    
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        ProcessNode(node->mChildren[i], scene, data, directory);
    }
}

void SkinnedModelLoader::ProcessMesh(const void* meshPtr, const void* scenePtr, SkinnedModelData& data) {
    const aiMesh* mesh = static_cast<const aiMesh*>(meshPtr);
    
    SkinnedSubMeshData submesh;
    submesh.Name = mesh->mName.C_Str();
    submesh.MaterialIndex = static_cast<int>(mesh->mMaterialIndex);
    
    // Reserve space
    submesh.Vertices.reserve(mesh->mNumVertices);
    
    // Process vertices
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        SkinnedVertex vertex;
        
        vertex.Position = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
        
        if (mesh->HasNormals()) {
            vertex.Normal = glm::vec3(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
        }
        
        if (mesh->mTextureCoords[0]) {
            vertex.TexCoord = glm::vec2(mesh->mTextureCoords[0][i].x, 1.0f - mesh->mTextureCoords[0][i].y);
        }
        
        if (mesh->HasTangentsAndBitangents()) {
            vertex.Tangent = glm::vec3(mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z);
            vertex.Bitangent = glm::vec3(mesh->mBitangents[i].x, mesh->mBitangents[i].y, mesh->mBitangents[i].z);
        }
        
        submesh.Vertices.push_back(vertex);
    }
    
    // Extract bone weights
    ExtractBones(mesh, data, submesh.Vertices);
    
    // Process indices
    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            submesh.Indices.push_back(face.mIndices[j]);
        }
    }
    
    // Normalize weights
    for (auto& vertex : submesh.Vertices) {
        vertex.NormalizeWeights();
    }
    
    SE_LOG_INFO("SkinnedModelLoader: Mesh '{}' - {} vertices, {} indices, hasBones: {}",
                submesh.Name, submesh.Vertices.size(), submesh.Indices.size(), mesh->HasBones());
    
    data.SubMeshes.push_back(std::move(submesh));
}

void SkinnedModelLoader::ExtractBones(const void* meshPtr, SkinnedModelData& data, std::vector<SkinnedVertex>& vertices) {
    const aiMesh* mesh = static_cast<const aiMesh*>(meshPtr);
    
    if (!mesh->HasBones()) {
        SE_LOG_INFO("SkinnedModelLoader: Mesh '{}' has no bones", mesh->mName.C_Str());
        return;
    }
    
    SE_LOG_INFO("SkinnedModelLoader: Extracting {} bones from mesh '{}'", mesh->mNumBones, mesh->mName.C_Str());
    
    for (unsigned int boneIdx = 0; boneIdx < mesh->mNumBones; ++boneIdx) {
        aiBone* bone = mesh->mBones[boneIdx];
        std::string boneName = bone->mName.C_Str();
        
        int boneId = data.GetBoneIndex(boneName);
        
        if (boneId < 0) {
            boneId = static_cast<int>(data.Bones.size());
            
            BoneInfo boneInfo;
            boneInfo.Name = boneName;
            boneInfo.Id = boneId;
            boneInfo.OffsetMatrix = ConvertMatrix(bone->mOffsetMatrix);
            
            data.Bones.push_back(boneInfo);
            data.BoneNameToIndex[boneName] = boneId;
            
            SE_LOG_INFO("SkinnedModelLoader: Added bone '{}' (id: {})", boneName, boneId);
        }
        
        // Apply bone weights to vertices
        for (unsigned int weightIdx = 0; weightIdx < bone->mNumWeights; ++weightIdx) {
            unsigned int vertexId = bone->mWeights[weightIdx].mVertexId;
            float weight = bone->mWeights[weightIdx].mWeight;
            
            if (vertexId < vertices.size()) {
                vertices[vertexId].AddBoneData(boneId, weight);
            }
        }
    }
}

void SkinnedModelLoader::ProcessBoneHierarchy(const void* nodePtr, SkinnedModelData& data, int parentIndex) {
    const aiNode* node = static_cast<const aiNode*>(nodePtr);
    std::string nodeName = node->mName.C_Str();
    
    int currentIndex = data.GetBoneIndex(nodeName);
    
    if (currentIndex >= 0) {
        data.Bones[currentIndex].ParentIndex = parentIndex;
        data.Bones[currentIndex].LocalTransform = ConvertMatrix(node->mTransformation);
        parentIndex = currentIndex;
    }
    
    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        ProcessBoneHierarchy(node->mChildren[i], data, parentIndex);
    }
}

void SkinnedModelLoader::ProcessMaterial(const void* materialPtr, SkinnedModelData& data, const std::string& directory) {
    const aiMaterial* material = static_cast<const aiMaterial*>(materialPtr);
    
    MaterialData matData;
    
    aiString name;
    if (material->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
        matData.Name = name.C_Str();
    }
    
    aiColor4D color;
    if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
        matData.DiffuseColor = glm::vec4(color.r, color.g, color.b, color.a);
    }
    
    matData.DiffuseTexturePath = FindTexture(material, aiTextureType_DIFFUSE, directory);
    matData.NormalTexturePath = FindTexture(material, aiTextureType_NORMALS, directory);
    if (matData.NormalTexturePath.empty()) {
        matData.NormalTexturePath = FindTexture(material, aiTextureType_HEIGHT, directory);
    }
    matData.SpecularTexturePath = FindTexture(material, aiTextureType_SPECULAR, directory);
    matData.AOTexturePath = FindTexture(material, aiTextureType_AMBIENT_OCCLUSION, directory);
    
    data.Materials.push_back(matData);
}

}  // namespace se
