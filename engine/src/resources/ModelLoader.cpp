#include "engine/resources/ModelLoader.h"

#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <assimp/Importer.hpp>
#include <filesystem>

#include "engine/core/Log.h"
#include "engine/renderer/Material.h"
#include "engine/renderer/Texture.h"
#include "engine/resources/MaterialManager.h"
#include "engine/resources/MeshManager.h"

namespace se {

namespace {
std::string g_directory;

std::shared_ptr<Texture> LoadTextureFromFile(const std::string& path) {
    return Texture::Create(path);
}

std::shared_ptr<Texture> LoadEmbeddedTexture(const aiTexture* aiTex) {
    if (!aiTex || !aiTex->pcData) {
        SE_LOG_WARN("[ModelLoader] Embedded texture has no data");
        return nullptr;
    }

    // For compressed textures (PNG/JPG), mHeight is 0 and mWidth contains the compressed size
    int dataSize = (aiTex->mHeight == 0) ? aiTex->mWidth : (aiTex->mWidth * aiTex->mHeight * 4);

    auto texture = Texture::CreateFromMemory(reinterpret_cast<const unsigned char*>(aiTex->pcData), dataSize);

    return texture;
}

std::filesystem::path ResolveTexturePath(const std::string& filename) {
    std::filesystem::path texPath(filename);
    std::filesystem::path fullPath;

    if (texPath.is_absolute()) {
        fullPath = std::filesystem::path(g_directory) / texPath.relative_path();
    } else {
        fullPath = std::filesystem::path(g_directory) / texPath;
    }

    return fullPath;
}

std::shared_ptr<Texture> LoadMaterialTexture(aiMaterial* aiMat, aiTextureType aiType, const aiScene* scene) {
    if (aiMat->GetTextureCount(aiType) == 0) return nullptr;

    aiString str;
    aiMat->GetTexture(aiType, 0, &str);
    std::string filename = std::string(str.C_Str());

    // Check for embedded texture (starts with *)
    if (!filename.empty() && filename[0] == '*') {
        int textureIndex = std::stoi(filename.substr(1));
        if (textureIndex < static_cast<int>(scene->mNumTextures)) {
            SE_LOG_INFO("[ModelLoader] Loading embedded texture index {}", textureIndex);
            return LoadEmbeddedTexture(scene->mTextures[textureIndex]);
        }
        return nullptr;
    }

    // External file
    auto fullPath = ResolveTexturePath(filename);
    if (std::filesystem::exists(fullPath)) {
        SE_LOG_INFO("[ModelLoader] Loading texture: {}", fullPath.string());
        return LoadTextureFromFile(fullPath.string());
    }

    SE_LOG_WARN("[ModelLoader] Texture file not found: {}", fullPath.string());
    return nullptr;
}

void LoadMaterialProperties(SubMesh& submesh, aiMaterial* aiMat) {
    aiColor3D color(1.0f, 1.0f, 1.0f);
    float     value;

    if (aiMat->Get(AI_MATKEY_COLOR_DIFFUSE, color) == AI_SUCCESS) { submesh.pbrProperties.Albedo = Vector3(color.r, color.g, color.b); }

    if (aiMat->Get(AI_MATKEY_COLOR_SPECULAR, color) == AI_SUCCESS) { submesh.pbrProperties.Specular = Vector3(color.r, color.g, color.b); }

    if (aiMat->Get(AI_MATKEY_SHININESS, value) == AI_SUCCESS) {
        submesh.pbrProperties.Shininess = value;
        // Convert shininess to roughness
        float roughness                 = 1.0f - (std::sqrt(value) / std::sqrt(100.0f));
        submesh.pbrProperties.Roughness = std::clamp(roughness, 0.05f, 1.0f);
    }

    if (aiMat->Get(AI_MATKEY_COLOR_EMISSIVE, color) == AI_SUCCESS) { submesh.pbrProperties.Emission = Vector3(color.r, color.g, color.b); }
}

void ProcessMesh(aiMesh* mesh, const aiScene* scene, std::shared_ptr<Model> model) {
    std::vector<float>        vertices;
    std::vector<unsigned int> indices;

    // Reserve space: 11 floats per vertex (pos 3, color 3, normal 3, texcoord 2)
    vertices.reserve(mesh->mNumVertices * 11);

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        // Position (3)
        vertices.push_back(mesh->mVertices[i].x);
        vertices.push_back(mesh->mVertices[i].y);
        vertices.push_back(mesh->mVertices[i].z);

        // Color (3)
        if (mesh->HasVertexColors(0)) {
            vertices.push_back(mesh->mColors[0][i].r);
            vertices.push_back(mesh->mColors[0][i].g);
            vertices.push_back(mesh->mColors[0][i].b);
        } else {
            vertices.push_back(1.0f);
            vertices.push_back(1.0f);
            vertices.push_back(1.0f);
        }

        // Normal (3)
        if (mesh->HasNormals()) {
            vertices.push_back(mesh->mNormals[i].x);
            vertices.push_back(mesh->mNormals[i].y);
            vertices.push_back(mesh->mNormals[i].z);
        } else {
            vertices.push_back(0.0f);
            vertices.push_back(1.0f);
            vertices.push_back(0.0f);
        }

        // TexCoord (2)
        if (mesh->HasTextureCoords(0)) {
            vertices.push_back(mesh->mTextureCoords[0][i].x);
            vertices.push_back(mesh->mTextureCoords[0][i].y);
        } else {
            vertices.push_back(0.0f);
            vertices.push_back(0.0f);
        }
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) { indices.push_back(face.mIndices[j]); }
    }

    // Create SubMesh
    SubMesh submesh;
    submesh.mesh = std::make_shared<Mesh>(vertices, indices);

    // Process Material
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial* aiMat = scene->mMaterials[mesh->mMaterialIndex];

        // Load PBR properties
        LoadMaterialProperties(submesh, aiMat);

        // Create material using default shader
        auto shader      = MaterialManager::GetDefaultMaterial()->GetShader();
        submesh.material = std::make_shared<Material>(shader);

        // Load textures by type
        submesh.diffuseTexture = LoadMaterialTexture(aiMat, aiTextureType_DIFFUSE, scene);
        if (!submesh.diffuseTexture) { submesh.diffuseTexture = LoadMaterialTexture(aiMat, aiTextureType_BASE_COLOR, scene); }
        submesh.normalTexture    = LoadMaterialTexture(aiMat, aiTextureType_NORMALS, scene);
        submesh.metallicTexture  = LoadMaterialTexture(aiMat, aiTextureType_METALNESS, scene);
        submesh.roughnessTexture = LoadMaterialTexture(aiMat, aiTextureType_DIFFUSE_ROUGHNESS, scene);
        submesh.aoTexture        = LoadMaterialTexture(aiMat, aiTextureType_AMBIENT_OCCLUSION, scene);
        submesh.emissionTexture  = LoadMaterialTexture(aiMat, aiTextureType_EMISSIVE, scene);

        // Bind diffuse texture to material
        if (submesh.diffuseTexture && RHI::IsValid(submesh.diffuseTexture->GetHandle())) {
            submesh.material->SetTexture("uTexture", submesh.diffuseTexture->GetHandle());
        }

        // TODO: Bind other PBR textures when shader supports them
        // submesh.material->SetTexture("uNormalMap", submesh.normalTexture->GetHandle());
        // etc.
    } else {
        submesh.material = MaterialManager::GetDefaultMaterial();
    }

    model->AddSubMesh(submesh);
}

void ProcessNode(aiNode* node, const aiScene* scene, std::shared_ptr<Model> model) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        ProcessMesh(mesh, scene, model);
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) { ProcessNode(node->mChildren[i], scene, model); }
}
}  // namespace

std::shared_ptr<Model> ModelLoader::Load(const std::string& path) {
    Assimp::Importer importer;
    const aiScene*   scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_GenNormals | aiProcess_FlipUVs | aiProcess_CalcTangentSpace |
                                                         aiProcess_OptimizeMeshes | aiProcess_GenUVCoords);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        SE_LOG_ERROR("[ModelLoader] Assimp error: {}", importer.GetErrorString());
        return nullptr;
    }

    SE_LOG_INFO("[ModelLoader] Loading model: {} ({} meshes, {} materials, {} textures)", path, scene->mNumMeshes, scene->mNumMaterials,
                scene->mNumTextures);

    g_directory = std::filesystem::path(path).parent_path().string();

    auto model = std::make_shared<Model>(std::filesystem::path(path).stem().string());
    ProcessNode(scene->mRootNode, scene, model);

    SE_LOG_INFO("[ModelLoader] Loaded {} submeshes from {}", model->GetMeshCount(), path);

    return model;
}

}  // namespace se
