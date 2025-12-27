#include "engine/resources/AssimpModelLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <filesystem>

#include "engine/Log.h"
#include "engine/resources/ModelData.h"

namespace se {

std::unique_ptr<ModelData> AssimpModelLoader::Load(const std::string& path) {
    SE_LOG_INFO("AssimpModelLoader: Loading model from '{}'", path);

    Assimp::Importer importer;
    
    unsigned int flags = aiProcess_Triangulate |  // Convert quads/n-gons to triangles
                         aiProcess_GenNormals |    // Generate flat normals if missing
                         aiProcess_FlipUVs |       // Flip Y-axis of texture coords for OpenGL
                         aiProcess_SortByPType;    // Split meshes by primitive type

    const aiScene* scene = importer.ReadFile(path, flags);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        SE_LOG_ERROR("AssimpModelLoader: Failed to load model '{}': {}", path, importer.GetErrorString());
        return nullptr;
    }

    auto modelData = std::make_unique<ModelData>();
    modelData->SourcePath = path;
    
    std::filesystem::path filePath(path);
    modelData->Name = filePath.stem().string();
    directory_ = filePath.parent_path().string();

    SE_LOG_INFO("AssimpModelLoader: Scene has {} meshes, {} materials", 
                scene->mNumMeshes, scene->mNumMaterials);

    for (unsigned int i = 0; i < scene->mNumMaterials; ++i) {
        modelData->Materials.push_back(ProcessMaterial(scene->mMaterials[i], directory_));
    }

    ProcessNode(scene->mRootNode, scene, *modelData);

    modelData->Bounds.Min = glm::vec3(std::numeric_limits<float>::max());
    modelData->Bounds.Max = glm::vec3(std::numeric_limits<float>::lowest());
    
    for (const auto& submesh : modelData->SubMeshes) {
        for (const auto& vertex : submesh.Vertices) {
            modelData->Bounds.Min = glm::min(modelData->Bounds.Min, vertex.Position);
            modelData->Bounds.Max = glm::max(modelData->Bounds.Max, vertex.Position);
        }
    }

    SE_LOG_INFO("AssimpModelLoader: Loaded model '{}' with {} submeshes, {} materials",
                modelData->Name, modelData->SubMeshes.size(), modelData->Materials.size());

    return modelData;
}

bool AssimpModelLoader::SupportsFormat(const std::string& extension) const {
    static const std::vector<std::string> supportedFormats = {
        ".fbx", ".obj", ".gltf", ".glb", ".dae", ".3ds", ".blend",
        ".stl", ".ply", ".x", ".ms3d", ".cob", ".smd", ".vta",
        ".mdl", ".md2", ".md3", ".pk3", ".mdc", ".x3d", ".lwo",
        ".lws", ".ac", ".ase", ".ifc", ".bvh", ".csm", ".dxf",
        ".hmp", ".irr", ".irrmesh", ".nff", ".ndo", ".off", ".q3o",
        ".q3s", ".raw", ".ter", ".xgl", ".zgl"
    };

    std::string ext = extension;
    if (!ext.empty() && ext[0] != '.') {
        ext = "." + ext;
    }
    
    for (auto& c : ext) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }

    for (const auto& format : supportedFormats) {
        if (format == ext) {
            return true;
        }
    }
    return false;
}

void AssimpModelLoader::ProcessNode(const aiNode* node, const aiScene* scene, ModelData& modelData) {
    for (unsigned int i = 0; i < node->mNumMeshes; ++i) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        modelData.SubMeshes.push_back(ProcessMesh(mesh, scene));
    }

    for (unsigned int i = 0; i < node->mNumChildren; ++i) {
        ProcessNode(node->mChildren[i], scene, modelData);
    }
}

SubMeshData AssimpModelLoader::ProcessMesh(const aiMesh* mesh, const aiScene* scene) {
    SubMeshData submesh;
    submesh.Name = mesh->mName.C_Str();
    submesh.MaterialIndex = static_cast<int>(mesh->mMaterialIndex);

    SE_LOG_INFO("AssimpModelLoader: Processing mesh '{}' with {} vertices, {} faces",
                submesh.Name, mesh->mNumVertices, mesh->mNumFaces);

    submesh.Vertices.reserve(mesh->mNumVertices);
    for (unsigned int i = 0; i < mesh->mNumVertices; ++i) {
        ModelVertex vertex;
        
        vertex.Position = glm::vec3(
            mesh->mVertices[i].x,
            mesh->mVertices[i].y,
            mesh->mVertices[i].z
        );

        if (mesh->HasNormals()) {
            vertex.Normal = glm::vec3(
                mesh->mNormals[i].x,
                mesh->mNormals[i].y,
                mesh->mNormals[i].z
            );
        }

        if (mesh->mTextureCoords[0]) {
            vertex.TexCoord = glm::vec2(
                mesh->mTextureCoords[0][i].x,
                mesh->mTextureCoords[0][i].y
            );
        }

        if (mesh->HasTangentsAndBitangents()) {
            vertex.Tangent = glm::vec3(
                mesh->mTangents[i].x,
                mesh->mTangents[i].y,
                mesh->mTangents[i].z
            );
            vertex.Bitangent = glm::vec3(
                mesh->mBitangents[i].x,
                mesh->mBitangents[i].y,
                mesh->mBitangents[i].z
            );
        }

        submesh.Vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; ++i) {
        aiFace& face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; ++j) {
            submesh.Indices.push_back(face.mIndices[j]);
        }
    }

    SE_LOG_INFO("AssimpModelLoader: Mesh '{}' processed: {} vertices, {} indices",
                submesh.Name, submesh.Vertices.size(), submesh.Indices.size());

    return submesh;
}

MaterialData AssimpModelLoader::ProcessMaterial(const aiMaterial* material, const std::string& directory) {
    MaterialData matData;

    aiString name;
    if (material->Get(AI_MATKEY_NAME, name) == AI_SUCCESS) {
        matData.Name = name.C_Str();
    }

    aiColor4D color;
    if (material->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) {
        matData.DiffuseColor = glm::vec4(color.r, color.g, color.b, color.a);
    }
    if (material->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) {
        matData.SpecularColor = glm::vec4(color.r, color.g, color.b, color.a);
    }
    if (material->Get(AI_MATKEY_COLOR_AMBIENT, color) == AI_SUCCESS) {
        matData.AmbientColor = glm::vec4(color.r, color.g, color.b, color.a);
    }

    float shininess;
    if (material->Get(AI_MATKEY_SHININESS, shininess) == AI_SUCCESS) {
        matData.Shininess = shininess;
    }

    aiString texPath;
    if (material->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
        std::filesystem::path fullPath = std::filesystem::path(directory) / texPath.C_Str();
        matData.DiffuseTexturePath = fullPath.string();
    }
    if (material->GetTexture(aiTextureType_NORMALS, 0, &texPath) == AI_SUCCESS) {
        std::filesystem::path fullPath = std::filesystem::path(directory) / texPath.C_Str();
        matData.NormalTexturePath = fullPath.string();
    }
    if (material->GetTexture(aiTextureType_SPECULAR, 0, &texPath) == AI_SUCCESS) {
        std::filesystem::path fullPath = std::filesystem::path(directory) / texPath.C_Str();
        matData.SpecularTexturePath = fullPath.string();
    }

    SE_LOG_INFO("AssimpModelLoader: Processed material '{}': diffuse={}, normal={}", 
                matData.Name,
                matData.DiffuseTexturePath.empty() ? "none" : matData.DiffuseTexturePath,
                matData.NormalTexturePath.empty() ? "none" : matData.NormalTexturePath);

    return matData;
}

}  // namespace se
